#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480
#define FRAME_PER_SECOND 240
#define SNAKE_SPEED 10
#define SNAKE_START_LEN 10
#define SNAKE_START_X (SCREEN_WIDTH/2)
#define SNAKE_START_Y (SCREEN_HEIGHT/2)
#define GRID_SIZE 8
#define SPEED_UP_TIME_INTERVAL 30	//in seconds
#define SPEED_UP_RATE 1.2	// speed multiplier: higher rate - faster, lower rate - slower
#define SLOW_DOWN_RATE 1.2	// speed multiplier: higher rate - slower, lower rate - faster
#define RED_DOT_LIVESPAN 10000 // in milliseconds
#define RED_DOT_FREQUENCY_LOWEST 5	//lowest time span between spawing red dot, in seconds
#define RED_DOT_FREQUENCY_HIGHEST 30	//highest time span between spawing red dot, in seconds
#define POINTS_PER_DOT 1

typedef struct {
	int x, y;
} Point;

typedef struct {
	Point* data;     // WskaŸnik na tablicê przechowuj¹c¹ elementy kolejki
	int front;      // Indeks pocz¹tku kolejki
	int end;       // Indeks koñca kolejki
	int size;       // Bie¿¹cy rozmiar kolejki
	int capacity;   // Maksymalna pojemnoœæ kolejki
} Queue;

typedef struct {
	int x;
	int y;
	int isBarActive;
	double progressBar;
	bool isLoaded;
	bool reset;
} DotStatus;

//=========================
//=====FUNKCJE KOLEJKI=====
//=========================

Queue* createQueue(int capacity) {
	Queue* queue = (Queue*)malloc(sizeof(Queue));
	queue->capacity = capacity;
	queue->front = 0;
	queue->size = 0;
	queue->end = capacity - 1;
	queue->data = (Point*)malloc(capacity * sizeof(Point));
	return queue;
}

bool isEmpty(Queue* queue) {
	return queue->size == 0;
}

bool isFull(Queue* queue) {
	return queue->size == queue->capacity;
}

void resizeQueue(Queue* queue) {
	int newCapacity = queue->capacity * 2; // Podwajamy pojemnoœæ
	Point* newData = (Point*)malloc(newCapacity * sizeof(Point));

	for (int i = 0; i < queue->size; i++) {
		newData[i] = queue->data[(queue->front + i) % queue->capacity];
	}

	free(queue->data);
	queue->data = newData;
	queue->capacity = newCapacity;
	queue->front = 0;
	queue->end = queue->size - 1;
}

void addToQueue(Queue* queue, Point item) {
	if (isFull(queue)) {
		resizeQueue(queue);
	}
	queue->end = (queue->end + 1) % queue->capacity; // Cyklowe przesuniêcie wskaŸnika koñca
	queue->data[queue->end] = item;
	queue->size++;
}

Point dequeue(Queue* queue) {
	Point empty = { -1, -1 };
	if (isEmpty(queue)) {
		return empty;
	}
	Point item = queue->data[queue->front];
	queue->front = (queue->front + 1) % queue->capacity;
	queue->size--;
	return item;
}

Point getElementAt(Queue* queue, int position) {
	if (position < 0 || position >= queue->size) {
		Point empty = { -1, -1 };
		return empty;
	}
	int index = (queue->front + position) % queue->capacity;
	return queue->data[index];
}

void freeQueue(Queue* queue) {
	if (queue->data != NULL) {
		free(queue->data);
		queue->data = NULL; // anty podwójne zwolnienie pamiêci
	}
	free(queue);
}

// narysowanie napisu txt na powierzchni screen, zaczynaj¹c od punktu (x, y)
// charset to bitmapa 128x128 zawieraj¹ca znaki
void DrawString(SDL_Surface *screen, int x, int y, const char *text,
                SDL_Surface *charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while(*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
		};
	};

// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt œrodka obrazka sprite na ekranie
void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};

// rysowanie pojedynczego pixela
void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32 *)p = color;
	};

// rysowanie linii o d³ugoœci l w pionie (gdy dx = 0, dy = 1) 
// b¹dŸ poziomie (gdy dx = 1, dy = 0)
void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for(int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
		};
	};

// rysowanie prostok¹ta o d³ugoœci boków l i k
void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k,
                   Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for(i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
	};

void DrawInfoBar(SDL_Surface* screen, SDL_Surface* charset, Uint32 elapsedTime, Uint32 white, Uint32 blue, double snakeSpeed, Uint32 speedUpTime, int totalPoints) {
	char text[128];

	DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 56, white, blue);

	sprintf(text, "Kacper Trznadel, 203563");
	DrawString(screen, 8, 8, text, charset);

	int minutes = elapsedTime / 60000;
	int seconds = (elapsedTime / 1000) % 60;
	int milliseconds = elapsedTime % 1000;
	sprintf(text, "Time: %02d:%02d:%03d", minutes, seconds, milliseconds);
	DrawString(screen, 8, 20, text, charset);
	sprintf(text, "Completed Requirements: 1, 2, 3, 4, A, B, C, D, E, F, G");
	DrawString(screen, 8, 32, text, charset);
	sprintf(text, "Points: %d", totalPoints);
	DrawString(screen, 8, 50, text, charset);
	//sprintf(text, "Snake Speed: %lf", snakeSpeed);
	//DrawString(screen, 200, 8, text, charset);
	//sprintf(text, "SpeedUp Speed: %d", speedUpTime);
	//DrawString(screen, 200, 20, text, charset);
}

void capFramerate(Uint32 starting_tick) {

	if (1000 / FRAME_PER_SECOND > SDL_GetTicks() - starting_tick) {
		SDL_Delay(1000 / FRAME_PER_SECOND - (SDL_GetTicks() - starting_tick));
	}

}

void DrawSnake(SDL_Surface* screen, Queue* snake, Uint32 color) {
	for (int i = 0; i < snake->size; i++) {
		Point segment = getElementAt(snake, i);

		if (i == snake->size - 1) {
			// G³owa wê¿a
			Uint32 headColor = SDL_MapRGB(screen->format, 255, 255, 0);
			DrawRectangle(screen, segment.x, segment.y, GRID_SIZE, GRID_SIZE, headColor, headColor);
		}
		else if (i == 0) {
			// Ogon wê¿a
			Uint32 tailColor = SDL_MapRGB(screen->format, 88, 57, 49);
			DrawRectangle(screen, segment.x, segment.y, GRID_SIZE / 2, GRID_SIZE / 2, tailColor, tailColor);
		}
		else {
			// Cia³o wê¿a z 'animacj¹'
			int isBig = ((snake->size - 1 - i) % 2 == 1);
			int size = isBig ? GRID_SIZE : GRID_SIZE / 2;

			// Wyœrodkowanie ma³ej komórki w GRID_SIZE
			int offsetX = isBig ? 0 : (GRID_SIZE - size) / 2;
			int offsetY = isBig ? 0 : (GRID_SIZE - size) / 2;

			DrawRectangle(screen, segment.x + offsetX, segment.y + offsetY, size, size, color, color);
		}
	}
}

void MoveSnake(Queue* snake, int dx, int dy) {
	Point head = getElementAt(snake, snake->size - 1);
	Point newHead = { head.x + dx, head.y + dy };
	addToQueue(snake, newHead);
	dequeue(snake);
}

bool CheckCollision(Point head, Queue* snake) {
	for (int i = 0; i < snake->size - 1; i++) {
		Point segment = getElementAt(snake, i);
		if (abs(head.x - segment.x) < GRID_SIZE && abs(head.y - segment.y) < GRID_SIZE) {
			return true;
		}
	}
	return false;
}

bool IsInsideGameArea(Point head) {
	return head.x >= 4 && head.x < SCREEN_WIDTH - 8 && head.y >= 62 && head.y < SCREEN_HEIGHT - 8;
}

void ResetQueue(Queue* queue) {
	queue->front = 0;
	queue->size = 0;
	queue->end = queue->capacity - 1;
	if (queue->data != NULL) {
		free(queue->data);
	}
	queue->data = (Point*)malloc(queue->capacity * sizeof(Point));

}

void RestartGame(Queue* snake, int* dx, int* dy, Uint32* gameStartTime, Uint32* lastMoveTime, double* snakeSpeed, Uint32* speedUpTime, int* totalPoints, DotStatus* blueDotInfo, DotStatus* redDotInfo) {
	// Resetowanie wê¿a
	ResetQueue(snake);
	*totalPoints = 0;
	*snake = *createQueue(100);
	*snakeSpeed = 1000 / SNAKE_SPEED;
	for (int i = 0; i < SNAKE_START_LEN; i++) {
		Point segment = { SNAKE_START_X, SNAKE_START_Y + i * GRID_SIZE };
		addToQueue(snake, segment);
	}

	// Resetowanie kierunku
	*dx = GRID_SIZE;
	*dy = 0;

	// Resetowanie czasu
	*gameStartTime = SDL_GetTicks();
	*lastMoveTime = *gameStartTime;
	*speedUpTime = *gameStartTime;

	// Resetowanie niebieskiej kropki
	blueDotInfo->x = -10;
	blueDotInfo->y = -10;
	blueDotInfo->isBarActive = 0;
	blueDotInfo->progressBar = 0.0;
	blueDotInfo->reset = true;

	// Resetowanie czerwonej kropki
	redDotInfo->x = -10;
	redDotInfo->y = -10;
	redDotInfo->isBarActive = 0;
	redDotInfo->progressBar = 0.0;
	redDotInfo->reset = true;
}

void InitSnake(Queue* snake) {
	for (int i = 0; i < SNAKE_START_LEN; i++) {
		Point segment = { SNAKE_START_X, SNAKE_START_Y + i * GRID_SIZE };
		addToQueue(snake, segment);
	}
}

void InitScreen(SDL_Surface* screen, SDL_Surface* charset, Uint32 color, SDL_Window* window) {
	SDL_FillRect(screen, NULL, color);
	SDL_SetColorKey(charset, SDL_TRUE, 0x000000);
	SDL_UpdateWindowSurface(window);
}

void HandleSnakeTurn(Point head, Point newHead, int* dx, int* dy) {
	if (newHead.x >= SCREEN_WIDTH - 12) { // Prawa krawêdŸ
		if (IsInsideGameArea({ head.x, head.y + GRID_SIZE })) {
			*dx = 0; *dy = GRID_SIZE; // Skrêt w dó³
		}
		else if (IsInsideGameArea({ head.x, head.y - GRID_SIZE })) {
			*dx = 0; *dy = -GRID_SIZE; // Skrêt w górê
		}
	}
	else if (newHead.x < 4) { // Lewa krawêdŸ
		if (IsInsideGameArea({ head.x, head.y - GRID_SIZE })) {
			*dx = 0; *dy = -GRID_SIZE; // Skrêt w górê
		}
		else if (IsInsideGameArea({ head.x, head.y + GRID_SIZE })) {
			*dx = 0; *dy = GRID_SIZE; // Skrêt w dó³
		}
	}
	else if (newHead.y < 62) { // Górna krawêdŸ
		if (IsInsideGameArea({ head.x + GRID_SIZE, head.y })) {
			*dx = GRID_SIZE; *dy = 0; // Skrêt w prawo
		}
		else if (IsInsideGameArea({ head.x - GRID_SIZE, head.y })) {
			*dx = -GRID_SIZE; *dy = 0; // Skrêt w lewo
		}
	}
	else if (newHead.y >= SCREEN_HEIGHT - 12) { // Dolna krawêdŸ
		if (IsInsideGameArea({ head.x - GRID_SIZE, head.y })) {
			*dx = -GRID_SIZE; *dy = 0; // Skrêt w lewo
		}
		else if (IsInsideGameArea({ head.x + GRID_SIZE, head.y })) {
			*dx = GRID_SIZE; *dy = 0; // Skrêt w prawo
		}
	}
}

void DrawProgressBar(SDL_Surface* screen, double progress) {
	Uint32 green = SDL_MapRGB(screen->format, 0, 255, 0);
	Uint32 white = SDL_MapRGB(screen->format, 255, 255, 255);

	int barWidth = 160;
	int filledWidth = (int)(barWidth * (1.0 - progress));

	DrawRectangle(screen, 474, 6, barWidth, 20, white, white);
	DrawRectangle(screen, 474, 6, filledWidth, 20, green, green);
}

Point DotGenerator(SDL_Surface* screen, Queue* snake) {

	Point random; random.x = -1; random.y = -1;
	int minX = 4 / GRID_SIZE, maxX = (SCREEN_WIDTH - 12) / GRID_SIZE, minY = 62 / GRID_SIZE, maxY = (SCREEN_HEIGHT - 12) / GRID_SIZE;

	do {
		random.x = (rand() % (maxX - minX) + minX) * GRID_SIZE;
		random.y = (rand() % (maxY - minY) + minY) * GRID_SIZE;
	} while (CheckCollision(random, snake) || !IsInsideGameArea(random));

	return random;
}

DotStatus BlueDotEvent(SDL_Surface* screen, Queue* snake, SDL_Surface* blueDot, int* totalPoints, DotStatus blueDotInfo, double* snakeSpeed) {

	static bool onMap = false;
	static Point dot = { -1, -1 };
	//SDL_Surface* scaledBlueDot = SDL_CreateRGBSurfaceWithFormat(0, 2, 2, blueDot->format->BitsPerPixel, blueDot->format->format);
	SDL_Surface* scaledBlueDot = SDL_LoadBMP("./blueDot4x4.bmp");

	if (blueDotInfo.isLoaded == true) {
		dot.x = blueDotInfo.x;
		dot.y = blueDotInfo.y;
		onMap = true;
		blueDotInfo.isLoaded = false;
	}
	else if (blueDotInfo.reset == true) {
		dot.x = blueDotInfo.x;
		dot.y = blueDotInfo.y;
		onMap = false;
		blueDotInfo.reset = false;
	}

	if (!onMap) {
		dot = DotGenerator(screen, snake);
		onMap = true;
		blueDotInfo.x = dot.x;
		blueDotInfo.y = dot.y;
	}

	if (CheckCollision(dot, snake)) {
		Point tail = getElementAt(snake, snake->size - 1);
		addToQueue(snake, tail);
		(*totalPoints) += POINTS_PER_DOT;
		*snakeSpeed /= 2;
		dot = DotGenerator(screen, snake);
		blueDotInfo.x = dot.x;
		blueDotInfo.y = dot.y;
	}
	else {
		static int pulse = 0;
		static int pulseDelay = 100;
		if ((pulse / pulseDelay) % 2 == 0) {
			DrawSurface(screen, blueDot, dot.x + GRID_SIZE / 2, dot.y + GRID_SIZE / 2);
		}
		else {
			DrawSurface(screen, scaledBlueDot, dot.x + GRID_SIZE / 2, dot.y + GRID_SIZE / 2);
		}

		pulse++;
		if (pulse >= pulseDelay * 2) { // Reset cyklu po wyœwietleniu obu form
			pulse = 0;
		}
	}

	blueDotInfo.isBarActive = 0;
	blueDotInfo.progressBar = 0.0;
	return blueDotInfo;

}

DotStatus RedDotEvent(SDL_Surface* screen, Queue* snake, SDL_Surface* redDot, double* snakeSpeed, Uint32 elapsedTime, int* totalPoints, DotStatus redDotInfo) {
	static bool onMap = false;       
	static Point dot = { -10, -10 };
	static Uint32 spawnTime = 0;     // Czas pojawienia siê kropki
	static Uint32 nextAppearanceTime = 0; // Czas nastêpnego pojawienia siê kropki
	static Uint32 randomDelay = 0;   // Losowe opóŸnienie przed pojawieniem siê kropki
	//SDL_Surface* scaledRedDot = SDL_CreateRGBSurfaceWithFormat(0, 2, 2, redDot->format->BitsPerPixel, redDot->format->format);
	SDL_Surface* scaledRedDot = SDL_LoadBMP("./redDot4x4.bmp");

	Uint32 currentTime = SDL_GetTicks();

	if (redDotInfo.isLoaded && redDotInfo.isBarActive) {
		dot.x = redDotInfo.x;
		dot.y = redDotInfo.y;
		onMap = redDotInfo.isBarActive;
		spawnTime = currentTime - (Uint32)(RED_DOT_LIVESPAN * redDotInfo.progressBar);
		redDotInfo.isLoaded = false;
	}
	else if (redDotInfo.isLoaded) {
		dot.x = -10;
		dot.y = -10;
		onMap = redDotInfo.isBarActive;
		nextAppearanceTime = 0;
		redDotInfo.isLoaded = false;
	}
	else if (redDotInfo.reset == true) {
		dot.x = -10;
		dot.y = -10;
		onMap = false;
		nextAppearanceTime = 0;
		redDotInfo.reset = false;
	}

	// Jeœli kropka nie jest na mapie
	if (!onMap) {
		if (nextAppearanceTime == 0) {
			// Ustaw pierwszy czas pojawienia siê
			randomDelay = (rand() % (RED_DOT_FREQUENCY_HIGHEST - RED_DOT_FREQUENCY_LOWEST + 1) + RED_DOT_FREQUENCY_LOWEST) * 1000;
			nextAppearanceTime = currentTime + randomDelay;
		}
		else if (currentTime >= nextAppearanceTime) {
			// Gdy nadejdzie czas, losuj pozycjê kropki
			dot = DotGenerator(screen, snake);
			redDotInfo.x = dot.x;
			redDotInfo.y = dot.y;
			spawnTime = currentTime;
			onMap = true;
		}
	}

	// Obliczanie postêpu paska
	double progress = 0.0;
	if (onMap) {
		progress = (double)(currentTime - spawnTime) / RED_DOT_LIVESPAN;
		redDotInfo.isBarActive = true;
	}
	else {
		redDotInfo.isBarActive = false;
	}

	// Jeœli kropka jest na mapie i czas jej ¿ycia min¹³
	if (onMap && progress >= 1.0) {
		onMap = false;
		nextAppearanceTime = currentTime + (rand() % (RED_DOT_FREQUENCY_HIGHEST - RED_DOT_FREQUENCY_LOWEST + 1) + RED_DOT_FREQUENCY_LOWEST) * 1000; // Ustaw kolejny czas pojawienia siê
	}

	// Jeœli w¹¿ zje kropkê
	if (onMap && CheckCollision(dot, snake)) {
		int random = rand() % 2;

		if (random)
			*snakeSpeed *= SLOW_DOWN_RATE;
		else
			dequeue(snake);

		(*totalPoints) += POINTS_PER_DOT;
		onMap = false;
		nextAppearanceTime = currentTime + (rand() % (RED_DOT_FREQUENCY_HIGHEST - RED_DOT_FREQUENCY_LOWEST + 1) + RED_DOT_FREQUENCY_LOWEST) * 1000; // Ustaw kolejny czas pojawienia siê
	}

	// Rysowanie kropki, jeœli jest na mapie
	if (onMap) {
		static int pulse = 0;
		static int pulseDelay = 50;
		if ((pulse / pulseDelay) % 2 == 0) {
			DrawSurface(screen, redDot, dot.x + GRID_SIZE / 2, dot.y + GRID_SIZE / 2);
		}
		else {
			DrawSurface(screen, scaledRedDot, dot.x + GRID_SIZE / 2, dot.y + GRID_SIZE / 2);
		}

		// Aktualizacja licznika pulsowania
		pulse++;
		if (pulse >= pulseDelay * 2) { // Reset cyklu po wyœwietleniu obu form
			pulse = 0;
		}
	}

	// Rysowanie paska postêpu
	if (onMap) {
		DrawProgressBar(screen, progress);
		redDotInfo.progressBar = progress;
	}

	return redDotInfo;
}

void RankingSorting(int points, int scores[3], char names[3][20], char newName[20]) {
	if (points > scores[2]) {
		strcpy(names[2], newName);
		scores[2] = points;
		for (int i = 0; i < 2; i++) {
			for (int j = i + 1; j < 3; j++) {
				if (scores[i] < scores[j]) {
					int tempScore = scores[i];
					scores[i] = scores[j];
					scores[j] = tempScore;

					char tempName[20];
					strcpy(tempName, names[i]);
					strcpy(names[i], names[j]);
					strcpy(names[j], tempName);
				}
			}
		}
	}

	FILE* ranking = fopen("ranking.txt", "w");
	if (ranking) {
		for (int i = 0; i < 3; i++) {
			fprintf(ranking, "%s %d\n", names[i], scores[i]);
		}
		fclose(ranking);
	}
}

void RankingDisplay(SDL_Surface* screen, SDL_Surface* charset, int points) {
	FILE* ranking;
	char names[3][20];
	int scores[3];

	// Odczyt rankingu z pliku
	ranking = fopen("ranking.txt", "r");
	if (ranking) {
		fscanf(ranking, "%s %d\n%s %d\n%s %d\n",
			names[0], &scores[0],
			names[1], &scores[1],
			names[2], &scores[2]);
		fclose(ranking);
	}
	else {
		// Domyœlne wartoœci rankingu
		strcpy(names[0], "N/A");
		strcpy(names[1], "N/A");
		strcpy(names[2], "N/A");
		scores[0] = scores[1] = scores[2] = 0;
	}

	// Wyœwietlenie rankingu
	DrawRectangle(screen, 100, 300, 440, 160, SDL_MapRGB(screen->format, 255, 255, 255), SDL_MapRGB(screen->format, 36, 36, 36));
	char text[128];
	sprintf(text, "Ranking:");
	DrawString(screen, 120, 320, text, charset);

	for (int i = 0; i < 3; i++) {
		sprintf(text, "%d. %s - %d", i + 1, names[i], scores[i]);
		DrawString(screen, 120, 340 + i * 20, text, charset);
	}

	// Sprawdzenie, czy wynik kwalifikuje siê do rankingu
	if (points > scores[2]) {
		sprintf(text, "Your score qualifies! Enter your name:");
		DrawString(screen, 120, 420, text, charset);
		SDL_UpdateWindowSurface(SDL_GetWindowFromID(1)); // Odœwie¿enie okna

		char newName[20] = "";
		SDL_StartTextInput();
		SDL_Event event;
		bool enteringName = true;

		while (enteringName && SDL_WaitEvent(&event)) {
			if (event.type == SDL_TEXTINPUT) {
				if (strlen(newName) < 19) {
					strcat(newName, event.text.text); // Dodawanie znaków
				}
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_RETURN) {
					enteringName = false; // Zatwierdzenie nicku Enterem
				}
				else if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(newName) > 0) {
					newName[strlen(newName) - 1] = '\0'; // Usuwanie znaku Backspace
				}
			}

			// Wyœwietlanie wprowadzanego nicku
			DrawRectangle(screen, 120, 440, 300, 18, SDL_MapRGB(screen->format, 255, 255, 255), SDL_MapRGB(screen->format, 36, 36, 36));
			DrawString(screen, 120, 445, newName, charset);
			SDL_UpdateWindowSurface(SDL_GetWindowFromID(1)); // Odœwie¿anie okna gry
		}
		SDL_StopTextInput();

		// Aktualizacja rankingu po wprowadzeniu nicku
		RankingSorting(points, scores, names, newName);

		// Wyœwietlenie zaktualizowanego rankingu
		DrawRectangle(screen, 100, 300, 440, 160, SDL_MapRGB(screen->format, 255, 255, 255), SDL_MapRGB(screen->format, 36, 36, 36));
		sprintf(text, "Updated Ranking:");
		DrawString(screen, 120, 320, text, charset);

		for (int i = 0; i < 3; i++) {
			sprintf(text, "%d. %s - %d", i + 1, names[i], scores[i]);
			DrawString(screen, 120, 340 + i * 20, text, charset);
		}
		SDL_UpdateWindowSurface(SDL_GetWindowFromID(1));
	}
}

void SaveGame(const char* filename, Queue* snake, int totalPoints, Uint32 elapsedTime, double snakeSpeed, int dx, int dy, DotStatus blueDotInfo, DotStatus redDotInfo) {
	FILE* file = fopen(filename, "w");
	if (file == NULL) {
		return;
	}

	fprintf(file, "%d\n", snake->size);
	for (int i = 0; i < snake->size; i++) {
		Point segment = getElementAt(snake, i);
		fprintf(file, "%d %d\n", segment.x, segment.y);
	}

	fprintf(file, "%d\n%u\n%lf\n%d\n%d\n", totalPoints, elapsedTime, snakeSpeed, dx, dy);

	fprintf(file, "%d %d %d %lf\n", blueDotInfo.x, blueDotInfo.y, blueDotInfo.isBarActive, blueDotInfo.progressBar);

	fprintf(file, "%d %d %d %lf\n", redDotInfo.x, redDotInfo.y, redDotInfo.isBarActive, redDotInfo.progressBar);

	fclose(file);
}

bool LoadGame(const char* filename, Queue* snake, int* totalPoints, Uint32* elapsedTime, double* snakeSpeed, int* dx, int* dy, DotStatus* blueDotInfo, DotStatus* redDotInfo, Uint32* speedUpTime) {
	FILE* file = fopen(filename, "r");
	if (file == NULL) {
		return false;
	}

	ResetQueue(snake);

	int size;
	if (fscanf(file, "%d", &size) != 1) {
		fclose(file);
		return false;
	}
	for (int i = 0; i < size; i++) {
		Point segment;
		if (fscanf(file, "%d %d", &segment.x, &segment.y) != 2) {
			fclose(file);
			return false;
		}
		addToQueue(snake, segment);
	}

	Uint32 oldTimer;

	if (fscanf(file, "%d %u %lf %d %d", totalPoints, &oldTimer, snakeSpeed, dx, dy) != 5) {
		fclose(file);
		return false;
	}

	if (fscanf(file, "%d %d %d %lf", &blueDotInfo->x, &blueDotInfo->y, &blueDotInfo->isBarActive, &blueDotInfo->progressBar) != 4) {
		fclose(file);
		return false;
	}

	if (fscanf(file, "%d %d %d %lf", &redDotInfo->x, &redDotInfo->y, &redDotInfo->isBarActive, &redDotInfo->progressBar) != 4) {
		fclose(file);
		return false;
	}

	Uint32 currentTime = SDL_GetTicks();
	*elapsedTime = currentTime - oldTimer;
	*speedUpTime = currentTime;
	*speedUpTime = *speedUpTime - (oldTimer % (1000*SPEED_UP_TIME_INTERVAL));

	blueDotInfo->isLoaded = 1;
	redDotInfo->isLoaded = 1;

	fclose(file);
	return true;
}

void HandleEvents(SDL_Event* event, bool* running, int* dx, int* dy, Queue* snake, Uint32* gameStartTime, Uint32* lastMoveTime, double* snakeSpeed, Uint32* speedUpTime, int* totalPoints, DotStatus* blueDotInfo, DotStatus* redDotInfo) {
	while (SDL_PollEvent(event)) {
		if (event->type == SDL_QUIT) {
			*running = false;
			break;
		}
		else if (event->type == SDL_KEYDOWN) {
			switch (event->key.keysym.sym) {
			case SDLK_ESCAPE: *running = false; break;
			case SDLK_UP: if (*dy == 0) { *dx = 0; *dy = -GRID_SIZE; } break;
			case SDLK_DOWN: if (*dy == 0) { *dx = 0; *dy = GRID_SIZE; } break;
			case SDLK_LEFT: if (*dx == 0) { *dx = -GRID_SIZE; *dy = 0; } break;
			case SDLK_RIGHT: if (*dx == 0) { *dx = GRID_SIZE; *dy = 0; } break;
			case SDLK_n: RestartGame(snake, dx, dy, gameStartTime, lastMoveTime, snakeSpeed, speedUpTime, totalPoints, blueDotInfo, redDotInfo); break;
			case SDLK_s: SaveGame("savegame.txt", snake, *totalPoints, SDL_GetTicks() - *gameStartTime, *snakeSpeed, *dx, *dy, *blueDotInfo, *redDotInfo); break;
			case SDLK_l: LoadGame("savegame.txt", snake, totalPoints, gameStartTime, snakeSpeed, dx, dy, blueDotInfo, redDotInfo, speedUpTime); break;
			}
		}
	}
}

void UpdateGame(Uint32* currentTime, Uint32* lastMoveTime, double* snakeSpeed, SDL_Event* event, SDL_Surface* screen, SDL_Surface* charset, SDL_Window* window, Queue* snake, int* dx, int* dy, int* totalPoints, Uint32* gameStartTime, Uint32* speedUpTime, DotStatus* blueDotInfo, DotStatus* redDotInfo, bool* running) {
	if (*currentTime - *lastMoveTime >= *snakeSpeed) {
		Point head = getElementAt(snake, snake->size - 1);
		Point newHead = { head.x + *dx, head.y + *dy };

		if (!IsInsideGameArea(newHead)) {
			HandleSnakeTurn(head, newHead, dx, dy);
		}

		newHead = { head.x + *dx, head.y + *dy };

		if (!IsInsideGameArea(newHead) || CheckCollision(newHead, snake)) {
			// Wyœwietlenie komunikatu koñca gry
			DrawRectangle(screen, 100, 200, 440, 80, SDL_MapRGB(screen->format, 255, 255, 255), SDL_MapRGB(screen->format, 36, 36, 36));
			DrawString(screen, 120, 220, "GAME OVER! Press 'n' to restart or 'ESC' to quit.", charset);
			RankingDisplay(screen, charset, *totalPoints);
			SDL_UpdateWindowSurface(window);

			// Pauza w grze czekaj¹ca na decyzjê gracza
			bool gameOver = true;
			while (gameOver && SDL_WaitEvent(event)) {
				if (event->type == SDL_QUIT || (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_ESCAPE)) {
					*running = false;
					gameOver = false;
				}
				else if (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_n) {
					RestartGame(snake, dx, dy, gameStartTime, lastMoveTime, snakeSpeed, speedUpTime, totalPoints, blueDotInfo, redDotInfo);
					*currentTime = SDL_GetTicks();
					gameOver = false;
				}
			}
		}
		MoveSnake(snake, *dx, *dy);
		*lastMoveTime = *currentTime;
	}
}

void FreeMemory(SDL_Surface* charset, SDL_Surface* screen, SDL_Surface* blueDot, SDL_Surface* redDot, Queue* snake, SDL_Window* window) {
	if (charset) SDL_FreeSurface(charset);
	if (screen) SDL_FreeSurface(screen);
	if (blueDot) SDL_FreeSurface(blueDot);
	if (redDot) SDL_FreeSurface(redDot);
	if (snake) freeQueue(snake);
	if (window) SDL_DestroyWindow(window);
	SDL_Quit();
}

// main
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char **argv) {
	
	SDL_Init(SDL_INIT_EVERYTHING);
	srand(time(NULL));
	
	SDL_Window* window = SDL_CreateWindow("Snake Game | Kacper Trznadel, 203563", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	SDL_Surface* screen = SDL_GetWindowSurface(window);
	SDL_Surface* charset = SDL_LoadBMP("./cs8x8.bmp");
	SDL_Surface* blueDot = SDL_LoadBMP("./blueDot.bmp");
	SDL_Surface* redDot = SDL_LoadBMP("./redDot.bmp");

	Uint32 black = SDL_MapRGB(screen->format, 0, 0, 0);
	Uint32 orange = SDL_MapRGB(screen->format, 255, 165, 0);
	Uint32 grey = SDL_MapRGB(screen->format, 36, 36, 36);
	Uint32 white = SDL_MapRGB(screen->format, 255, 255, 255);
	Uint32 blue = SDL_MapRGB(screen->format, 0, 0, 255);

	Queue* snake = createQueue(100);
	InitSnake(snake);
	InitScreen(screen, charset, black, window);

	int dx = GRID_SIZE, dy = 0;
	int totalPoints = 0;
	SDL_Event event;
	bool running = true;
	Uint32 starting_tick;
	Uint32 gameStartTime, lastMoveTime, speedUpTime;
	DotStatus blueDotInfo = { -10,-10,0,0,0 };
	DotStatus redDotInfo = { -10,-10,0,0,0 };
	double snakeSpeed = 1000 / SNAKE_SPEED;
	gameStartTime = lastMoveTime = speedUpTime = SDL_GetTicks();

	while (running) {

		HandleEvents(&event, &running, &dx, &dy, snake, &gameStartTime, &lastMoveTime, &snakeSpeed, &speedUpTime, &totalPoints, &blueDotInfo, &redDotInfo);

		Uint32 currentTime = SDL_GetTicks();
		UpdateGame(&currentTime, &lastMoveTime, &snakeSpeed, &event, screen, charset, window, snake, &dx, &dy, &totalPoints, &gameStartTime, &speedUpTime, &blueDotInfo, &redDotInfo, &running);

		starting_tick = SDL_GetTicks();
		Uint32 elapsedTime = (starting_tick - gameStartTime); // Czas w milisekundach

		if ((currentTime - speedUpTime)/1000 >= SPEED_UP_TIME_INTERVAL) {
			snakeSpeed /= SPEED_UP_RATE;
			speedUpTime = currentTime;
		}

		DrawInfoBar(screen, charset, elapsedTime, white, blue, snakeSpeed, speedUpTime, totalPoints);
		DrawRectangle(screen, 4, 62, SCREEN_WIDTH - 8, SCREEN_HEIGHT - 64, white, grey);
		blueDotInfo = BlueDotEvent(screen, snake, blueDot, &totalPoints, blueDotInfo, &snakeSpeed);
		redDotInfo = RedDotEvent(screen, snake, redDot, &snakeSpeed, elapsedTime, &totalPoints, redDotInfo);
		DrawSnake(screen, snake, orange);
		SDL_UpdateWindowSurface(window);
		capFramerate(starting_tick);
	}

	FreeMemory(charset, screen, blueDot, redDot, snake, window);
	return 0;
	};
