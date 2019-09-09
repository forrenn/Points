#include <iostream>
#include <SDL/SDL.h>
#include <vector>

#pragma comment(lib,"SDL2.lib")
#undef main

struct MyPoint
{
	float r, g, b;
	int x, y;
};
void main()
{
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window* window;
	SDL_Renderer* renderer;
	int w = 1280;
	int h = 720;

	SDL_CreateWindowAndRenderer(w, h, 0, &window, &renderer);

	SDL_Event ev;
	std::vector<MyPoint> points;
	for (int i = 0; i < w*h; i++)
	{
		float r = rand();
		float g = rand();
		float b = rand();
		r /= RAND_MAX;
		g /= RAND_MAX;
		b /= RAND_MAX;
		int x = i % w;
		int y = i / w;

		points.emplace_back(r, g, b, x, y);
	}
	while (true)
	{
		while (SDL_PollEvent(&ev))
		{

		}
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		std::vector<SDL_Point> points;
		for (int i = 0; i<1024*104; ++i) points.push_back({ rand() % w, rand() % h });

		SDL_RenderDrawPoints(renderer, points.data(), points.size());
		SDL_RenderPresent(renderer);
	}

	system("pause");
}