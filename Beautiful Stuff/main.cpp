#include <iostream>
#include <SDL/SDL.h>
#include <vector>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <SDL/SDL_image.h>
#include <string>
#include <thread>
#include "MyPoint.h"

#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2_image.lib")
#undef main

double LOSS = 0.001; //percentage of color lost for each pixel of distance (multiplicative)
float LOSS_MULT = (100 - LOSS) / 100;
double REFRESH_RATE = 60;

double REFRESH_PERIOD = 1.0 / REFRESH_RATE;

uint64_t xorshift64(uint64_t* state)
{
	uint64_t x = *state;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	return *state = x;
}

MyPoint* getPointByCoords(std::vector<MyPoint>& pts, int x, int y, int w, int h)
{
	if (x < 0 || y < 0 || x >= w || y >= h) return nullptr;
	else
		return &pts[y*w + x];
}

void routine(std::vector<MyPoint>& points, int workerNumber, int maxWorkers, int w, int h, uint64_t rndSeed, bool& running)
{
	int startX = 0;
	int workerPixels = h / maxWorkers;
	int startY = workerNumber * workerPixels;
	int endX = w;
	int endY = startY + workerPixels;

	auto it = points.begin() + (startY*w);
	for (int y = startY; y < endY; ++y)
	{
		for (int x = startX; x < endX; ++x)
		{
			MyPoint& p = *it;

			MyPoint* neighbor = nullptr;
			int neighborX, neighborY;
			while (!neighbor) //find the right neighbor
			{
				neighborX = x;
				neighborY = y;
				uint8_t side = xorshift64(&rndSeed) % 8;
				static const int8_t table1[] = { -1,-1,-1,0,0,1,1,1 }; //notice the abscence of (0,0) here
				static const int8_t table2[] = { -1,0,1,-1,1,-1,0,1 }; //and here
				neighborX += table1[side];
				neighborY += table2[side];
				neighbor = getPointByCoords(points, neighborX, neighborY, w, h);
				if (neighbor == &p) neighbor = nullptr; //DON'T REMOVE THIS, strange speedup on AMD FX
			}

			uint64_t r1 = xorshift64(&rndSeed);
			if (r1 & 1) *neighbor = p;
			else p = *neighbor;

			++it;
		}
	}
}

void setPixel(SDL_Surface* s, int x, int y, uint32_t r, uint32_t g, uint32_t b)
{
	SDL_PixelFormat* format = s->format;
	Uint32 color = 0;
	color |= b << format->Bshift;
	color |= g << format->Gshift;
	color |= r << format->Rshift;

	char* px = (char*)s->pixels + s->pitch*y + x * format->BytesPerPixel;
	*(Uint32*)px = color;
}

void main()
{
	int threadCount;
	SDL_Init(SDL_INIT_EVERYTHING);
	/*if (true) //for tests, remove when releasing!!!
	{
		std::string testStr = "1\r\n1\r\n1280\r\n720\r\n";
		for (auto c : testStr)
			std::cin.putback(c);
	}*/

	int w = 1280;
	int h = 720;
	std::string path;
	uint64_t xorshift_state = 1;
	std::cout << "Enter desired base image path or invalid path for random points: ";
	std::getline(std::cin, path);

	std::cout << "Enter RNG seed: ";
	std::cin >> xorshift_state;

	std::cout << "Enter desired window width (0 for default value of 1024): ";
	std::cin >> w;
	if (w == 0) w = 1024;
	std::cout << "Enter desired window height (0 for default value of 600): ";
	std::cin >> h;
	if (h == 0) h = 600;
	std::cout << "Enter desired thread count (0 for auto detect): ";
	std::cin >> threadCount;
	if (threadCount == 0) threadCount = std::thread::hardware_concurrency();

	if (h % threadCount != 0)
	{
		std::cout << "Height is not a multiple of thread count (" << threadCount << "), increasing it to ";
		h += threadCount - (h % threadCount);
		std::cout << h << "\n";
	}	

	SDL_Window* window;
	SDL_Surface* windowSurface;

	window = SDL_CreateWindow("Point communism", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, 0);
	windowSurface = SDL_GetWindowSurface(window);
	std::vector<int> colorsCounter(16777216), colorsCounterOld(16777216);
	SDL_Event ev;
	std::vector<MyPoint> points;

	SDL_Surface* img = IMG_Load(path.c_str());
	SDL_BlitScaled(img, &img->clip_rect, windowSurface, &windowSurface->clip_rect);
	
	if (img)
	{
		Uint32* pixels = (Uint32*)windowSurface->pixels;
		SDL_PixelFormat* format = windowSurface->format;
		for (int i = 0; i < w*h; i++)
		{
			Uint32 px = *pixels++;
			uint8_t r = (px & format->Rmask) >> format->Rshift;
			uint8_t g = (px & format->Gmask) >> format->Gshift;
			uint8_t b = (px & format->Bmask) >> format->Bshift;

			points.push_back({ r, g, b });
			int index = r + g * 256 + b * 65536;
			colorsCounterOld[index]++;
		}
	}
	else
	{
		std::cout << "Can't open " << path << ", generating random points...\n";
		for (int i = 0; i < w*h; i++)
		{
			uint8_t r = xorshift64(&xorshift_state);
			uint8_t g = xorshift64(&xorshift_state);
			uint8_t b = xorshift64(&xorshift_state);

			points.push_back({ r, g, b });
			int index = r + g * 256 + b * 65536;
			colorsCounterOld[index]++;
		}
	}

	uint64_t frames = 0;
	auto startTime = std::chrono::high_resolution_clock::now();
	double refreshAccumulator = 0;
	
	bool running = true;
	std::vector<std::thread> workers(threadCount);

	while (running)
	{
		auto currFrameStartTime = std::chrono::high_resolution_clock::now();
		while (SDL_PollEvent(&ev))
		{
			if (ev.type == SDL_QUIT)
				return;
		}		

		for (int i = 0; i < threadCount; ++i)
		{
			workers[i] = std::thread(routine, std::ref(points), i, threadCount, w, h, xorshift64(&xorshift_state), std::ref(running));
		}
		for (auto& it_t : workers) it_t.join();

		++frames;
		auto endTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsedTime = endTime - startTime;
		double totalTime = elapsedTime.count();
		
		std::chrono::duration<double> frameTimeChrono = endTime - currFrameStartTime;
		double frameTime = frameTimeChrono.count();

		auto windowFlags = SDL_GetWindowFlags(window);
		if (!(windowFlags & SDL_WINDOW_MINIMIZED))
			refreshAccumulator += frameTime;

		if (refreshAccumulator > REFRESH_PERIOD) //sparse refreshing (don't refresh faster than predetermined rate)
		{
			SDL_FillRect(windowSurface, nullptr, 0);
			auto it = points.begin();
			for (int y = 0; y < h; ++y)
			{
				for (int x = 0; x < w; ++x)
				{
					setPixel(windowSurface, x, y, it->r, it->g, it->b);
					++it;
				}
			}

			SDL_UpdateWindowSurface(window);
			refreshAccumulator -= REFRESH_PERIOD;
			if (refreshAccumulator > REFRESH_PERIOD) refreshAccumulator = 0;
		}

		if (frames % 200 == 0) std::cout << "Frame " << frames << ", FPS: inst: " << 1/frameTime << ", avg: " << frames / totalTime << "\n";

		if (frames % 400 == 0)
		{
			for (auto& it : colorsCounter) it = 0;
			for (auto it : points)
			{
				int index = it.r + it.g * 256 + it.b * 65536;
				colorsCounter[index]++;
			}
			int totalColors = 0;
			for (const auto& it : colorsCounter)
				totalColors += it > 0;
			std::cout << totalColors << " unique colors remaining\n";

			static std::ofstream fs("out.csv");
			fs << totalColors << std::endl;

			if (totalColors < 100)
			{
				for (int i = 0; i < 16777216; ++i)
					if (colorsCounterOld[i] > 0 && colorsCounter[i] == 0)
					{
						int r = i & 0xFF;
						int g = (i & 0xFF00) >> 8;
						int b = (i & 0xFF0000) >> 16;
						std::cout << "RIP RGB " << r << " " << g << " " << b << " at frame " << frames << "\n";
					}						
			}
			colorsCounterOld = colorsCounter;

			if (totalColors == 1)
			{
				running = false;
				for (int i = 0; i < 16777216; ++i)
					if (colorsCounter[i] != 0)
					{
						int r = i & 0xFF;
						int g = (i & 0xFF00) >> 8;
						int b = (i & 0xFF0000) >> 16;
						std::cout << "Winner is: RGB " << r << " " << g << " " << b << " at frame " << frames << "\n";
					}						
			}
		}
	}

	system("pause");
}