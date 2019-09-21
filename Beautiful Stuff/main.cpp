#include <iostream>
#include <SDL/SDL.h>
#include <vector>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <SDL/SDL_image.h>

#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2_image.lib")
#undef main

double LOSS = 0.001; //percentage of color lost for each pixel of distance (multiplicative)
float LOSS_MULT = (100 - LOSS) / 100;
double REFRESH_RATE = 60;

double REFRESH_PERIOD = 1.0 / REFRESH_RATE;

uint64_t xorshift64()
{
	static uint64_t a = 0xFA38298A249B2439;
	uint64_t x = a;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	return a = x;
}

struct MyPoint
{
	uint8_t r, g, b;
};

void setPixel(SDL_Surface* s, int x, int y, Uint32 c)
{
	int offset = y * s->pitch + x * 4;
	memcpy((char*)s->pixels + offset, &c, 4);
}

void setPixel(SDL_Surface* s, int x, int y, uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
	Uint32 color = 0;
	color |= a << 24;
	color |= b << 16;
	color |= g << 8;
	color |= r;
	setPixel(s, x, y, color);
}
void setPixel(SDL_Surface* s, int x, int y, uint32_t r, uint32_t g, uint32_t b)
{
	SDL_PixelFormat* format = s->format;
	Uint32 color = 0;
	color |= b << format->Bshift;
	color |= g << format->Gshift;
	color |= r << format->Rshift;
	//color |= 255 << format->Ashift;
	setPixel(s, x, y, color);
}

MyPoint* getPointByCoords(std::vector<MyPoint>& pts, int x, int y, int w, int h)
{
	if (x < 0 || y < 0 || x >= w || y >= h) return nullptr;
	else
		return &pts[y*w + x];
}

double getRandomWeightedValue()
{
	double n = double(xorshift64()) / uint64_t(-1);
	//return -log2(n);
	return 1 / n;
}

void main()
{
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window* window;
	SDL_Surface* windowSurface;
	//	SDL_Renderer* renderer;
	int w = 1280;
	int h = 720;

	//SDL_CreateWindowAndRenderer(w, h, 0, &window, &renderer);
	window = SDL_CreateWindow("Point communism", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, 0);
	windowSurface = SDL_GetWindowSurface(window);

	SDL_Surface* img = IMG_Load("e:/fc/img/space-planet-surface-wallpaper.jpg");
	SDL_BlitScaled(img, &img->clip_rect, windowSurface, &windowSurface->clip_rect);

	std::vector<int> colorsCounter(16777216), colorsCounterOld(16777216);

	SDL_Event ev;
	std::vector<MyPoint> points;
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

	uint64_t frames = 0;
	auto startTime = std::chrono::high_resolution_clock::now();
	double refreshAccumulator = 0;

	while (true)
	{
		auto currFrameStartTime = std::chrono::high_resolution_clock::now();
		while (SDL_PollEvent(&ev))
		{

		}
		SDL_FillRect(windowSurface, 0, 0);

		auto it = points.begin();
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				MyPoint& p = *it;

				MyPoint* neighbor = nullptr;
				int neighborX, neighborY;
				while (!neighbor) //find the right neighbor
				{
					neighborX = x;
					neighborY = y;
					uint8_t side = xorshift64() % 9;		
					static const int8_t table1[] = { -1,-1,-1,0,0,0,1,1,1 };
					static const int8_t table2[] = { -1,0,1,-1,0,1,-1,0,1 };
					neighborX += table1[side];
					neighborY += table2[side];
					neighbor = getPointByCoords(points, neighborX, neighborY, w, h);
					if (neighbor == &p) neighbor = nullptr;
				}
				
				uint64_t r1 = xorshift64();
				uint64_t r2 = xorshift64();
				if (r1 > r2)
				{
					*neighbor = p;
					//setPixel(windowSurface, neighborX, neighborY, p.r, p.g, p.b);
				}
				else
				{
					p = *neighbor;
					//setPixel(windowSurface, x, y, neighbor->r, neighbor->g, neighbor->b);
				}


				//setPixel(windowSurface, x, y, p.r * 255, p.g * 255, p.b * 255);
				++it;
			}
		}

		it = points.begin();
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				setPixel(windowSurface, x, y, it->r, it->g, it->b);
				//setPixel(windowSurface, x, y, 255, 0, 0);
				++it;
			}
		}
		++frames;
		auto endTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsedTime = endTime - startTime;
		double totalTime = elapsedTime.count();
		
		std::chrono::duration<double> frameTimeChrono = endTime - currFrameStartTime;
		double frameTime = frameTimeChrono.count();

		refreshAccumulator += frameTime;
		if (refreshAccumulator > REFRESH_PERIOD)
		{
			SDL_UpdateWindowSurface(window);
			refreshAccumulator -= REFRESH_PERIOD;
			if (refreshAccumulator > REFRESH_PERIOD) refreshAccumulator = 0;
		}

		if (frames % 100 == 0) std::cout << "FPS: inst: " << 1/frameTime << ", avg: " << frames / totalTime << "\n";

		if (frames % 200 == 0)
		{
			for (auto& it : colorsCounter) it = 0;
			for (auto it : points)
			{
				int index = it.r + it.g * 256 + it.b * 65536;
				colorsCounter[index]++;
			}
			int totalColors = 0;
			for (auto& it : colorsCounter)
				totalColors += it > 0;
			std::cout << totalColors << " unique colors remaining\n";

			static std::ofstream fs("out.csv");
			fs << totalColors << std::endl;

			if (totalColors < 100)
			{
				for (int i = 0; i < 16777216; ++i)
				{
					if (colorsCounterOld[i] > 0 && colorsCounter[i] == 0)
					{
						std::cout << "RIP RGB " << (i & 0xFF) << " " << ((i & 0xFF00) >> 8) << " " << ((i & 0xFF0000) >> 16) << " at frame " << frames << "\n";
					}
				}
			}
			colorsCounterOld = colorsCounter;
		}
	}

	system("pause");
}