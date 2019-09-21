#include <iostream>
#include <SDL/SDL.h>
#include <vector>
#include <algorithm>
#include <chrono>

#pragma comment(lib,"SDL2.lib")
#undef main

double LOSS = 0.001; //percentage of color lost for each pixel of distance (multiplicative)
float LOSS_MULT = (100 - LOSS) / 100;

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
	float r, g, b;

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
	Uint32 color = 0xFF000000;
	color |= b << 16;
	color |= g << 8;
	color |= r;
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

	SDL_Event ev;
	std::vector<MyPoint> points;
	for (int i = 0; i < w*h; i++)
	{
		float r = xorshift64();
		float g = xorshift64();
		float b = xorshift64();
		r /= uint64_t(-1);
		g /= uint64_t(-1);
		b /= uint64_t(-1);
		int x = i % w;
		int y = i / w;

		points.push_back({ r, g, b });
	}

	uint64_t frames = 0;
	auto startTime = std::chrono::high_resolution_clock::now();
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
				MyPoint& p = *getPointByCoords(points, x, y, w, h);

				MyPoint* neighbor = nullptr;
				int neighborX, neighborY;
				while (!neighbor) //find the right neighbor
				{
					neighborX = x;
					neighborY = y;
					uint8_t side = xorshift64() % 4;					
					neighborX += side & 1 ? 1 : -1;
					neighborY += side & 2 ? 1 : -1;
					neighbor = getPointByCoords(points, neighborX, neighborY, w, h);
				}
				
				uint64_t r1 = xorshift64();
				uint64_t r2 = xorshift64();
				if (r1 & 1)
				{
					*neighbor = p;
					setPixel(windowSurface, neighborX, neighborY, p.r * 255, p.g * 255, p.b * 255);
				}
				else
				{
					p = *neighbor;
					setPixel(windowSurface, x, y, neighbor->r * 255, neighbor->g * 255, neighbor->b * 255);
				}

				//setPixel(windowSurface, x, y, p.r * 255, p.g * 255, p.b * 255);
				++it;
			}
		}

		SDL_UpdateWindowSurface(window);
		++frames;
		auto endTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsedTime = endTime - startTime;
		double totalTime = elapsedTime.count();
		
		std::chrono::duration<double> frameTimeChrono = endTime - currFrameStartTime;
		double frameTime = frameTimeChrono.count();

		std::cout << "FPS: inst: " << 1/frameTime << ", avg: " << frames / totalTime << "\n";
	}

	system("pause");
}