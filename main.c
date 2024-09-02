#include <SDL2/SDL.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

uint8_t sprites[80] = {
  0xF0,0x90,0x90,0x90,0xF0,
  0x20,0x60,0x20,0x20,0x70,
  0xF0,0x10,0xF0,0x80,0xF0,
  0xF0,0x10,0xF0,0x10,0xF0,
  0x90,0x90,0xF0,0x10,0x10,
  0xF0,0x80,0xF0,0x10,0xF0,
  0xF0,0x80,0xF0,0x90,0xF0,
  0xF0,0x10,0x20,0x40,0x40,
  0xF0,0x90,0xF0,0x90,0xF0,
  0xF0,0x90,0xF0,0x10,0xF0,
  0xF0,0x90,0xF0,0x90,0x90,
  0xE0,0x90,0xE0,0x90,0xE0,
  0xF0,0x80,0x80,0x80,0xF0,
  0xE0,0x90,0x90,0x90,0xE0,
  0xF0,0x80,0xF0,0x80,0xF0,
  0xF0,0x80,0xF0,0x80,0x80
};

typedef struct {
  uint8_t ram[4096];
  uint8_t V[16];
  uint8_t sound;
  uint8_t delay;
  uint8_t SP;
  uint16_t stack[16];
  uint16_t I;
  uint16_t PC;
  uint16_t code;
  bool isRunning;
} CPU;

typedef struct {
  SDL_DisplayMode displayMode;
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* texture;
  uint16_t h, w;
  uint8_t pixels[];
} Display;

Display* displayCreate();
void displayClear(Display* display);
void displayRender(Display* display);
void displayDestroy(Display* display);

void cpuLoadCode(CPU* cpu, const char* filepath);
void cpuNext(CPU* cpu, Display* display);

int main(int argc, char *argv[]) {
  CPU cpu = {.PC=512,.isRunning=true};

  memcpy(cpu.ram, sprites, sizeof(sprites));

  assert(argc >= 2 && "Please provide path to a .ch8 emu file");
  cpuLoadCode(&cpu, argv[1]);

  Display* display = displayCreate();

  if (!display) {
    const char* error = SDL_GetError();
    fprintf(stderr, "failed to create a display: %s", error);
    displayDestroy(display);
    return -1;
  }

  while (cpu.isRunning) {
    cpuNext(&cpu, display);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        cpu.isRunning = false;
        break;
      }
      if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
        cpu.isRunning = false;
        break;
      }
    }
    
    displayRender(display);
    SDL_Delay(10);
  }
  
  displayDestroy(display);

  return 0;
}



Display* displayCreate() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
    fprintf(stderr, "failed to init sdl");
    return NULL;
  }

  SDL_DisplayMode displayMode;
  
  if (SDL_GetDesktopDisplayMode(0, &displayMode)) {
    fprintf(stderr, "failed to get display mode");
    return NULL;
  }
  
  Display* display = malloc(sizeof(Display) + 64 * 32);

  display->h = displayMode.h / 3;
  display->w = displayMode.w / 3;

  display->window = SDL_CreateWindow("Chip8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, display->w, display->h, 0);
  
  if (!display->window) {
    fprintf(stderr, "failed to create a window");
    return NULL;
  }

  display->renderer = SDL_CreateRenderer(display->window, -1, SDL_RENDERER_PRESENTVSYNC);

  if (!display->renderer) {
    fprintf(stderr, "failed to create a renderer");
    return NULL;
  }
  
  display->texture = SDL_CreateTexture(display->renderer, SDL_PIXELFORMAT_RGB332, SDL_TEXTUREACCESS_STREAMING, 64, 32);

  if (!display->texture) {
    fprintf(stderr, "failed to create a texture");
    return NULL;
  }

  return display;
}

void displayClear(Display* display) {
  memset(display->pixels, 0, 32 * 64);
}

void displayRender(Display* display) {
  SDL_SetRenderDrawColor(display->renderer, 0, 0, 0, 0);
  SDL_RenderClear(display->renderer);
  SDL_UpdateTexture(display->texture, NULL, display->pixels, 64 * sizeof(uint8_t));
  SDL_RenderCopy(display->renderer, display->texture, NULL, NULL);
  SDL_RenderPresent(display->renderer);
}

void displayDestroy(Display* display) {
  if (display->texture) {
    SDL_DestroyTexture(display->texture);
  }

  if (display->renderer) {
    SDL_DestroyRenderer(display->renderer);
  }

  if (display->window) {
    SDL_DestroyWindow(display->window);
  }

  SDL_Quit();
  free(display);
}

void cpuLoadCode(CPU* cpu, const char* filepath) {
  FILE* file = fopen(filepath, "r");
  uint16_t code = cpu->PC;

  int a;
  while ((a = fgetc(file)) != EOF) {
    cpu->ram[code] = a;
    code++;
  }

  fclose(file);
}

void cpuNext(CPU* cpu, Display* display) {
    uint16_t instr = (cpu->ram[cpu->PC] << 8) | cpu->ram[cpu->PC + 1];
    cpu->PC += 2;
    
    uint16_t addr = instr & 0x0FFF;
    uint8_t kk = instr & 0x00FF;
    uint8_t n = instr & 0x000F;

    uint8_t x = (instr >> 8) & 0x000F; 
    uint8_t y = (instr >> 4) & 0x000F;

    uint8_t opcode = (instr >> 12) & 0x000F; 

    switch (opcode) {
      case 0x0:
        switch (instr) {
          //CLS
          case 0x00E0: 
            displayClear(display);
            break;
          //RET
          case 0x00EE:
            cpu->PC = cpu->stack[cpu->SP];
            cpu->SP -= 1;
            break;
          //SYS
          default:
            cpu->PC = addr;
            break;
        };
        break;
      //JP
      case 0x1:
        cpu->PC = addr;
        break;
      //CALL
      case 0x2:
        cpu->SP += 1;
        cpu->stack[cpu->SP] = cpu->PC; 
        cpu->PC = addr;
        break;
      //SE Vx, byte
      case 0x3:
        if (cpu->V[x] == kk) {
          cpu->PC += 2;
        }
        break;
      //SNE Vx, byte
      case 0x4:
        if (cpu->V[x] != kk) {
          cpu->PC += 2;
        }
        break;
      //SE Vx, Vy
      case 0x5:
        if (cpu->V[x] == cpu->V[y]) {
          cpu->PC += 2;
        }
        break;
      //LD Vx, byte
      case 0x6:
        cpu->V[x] = kk;
        break;
      //ADD Vx, byte
      case 0x7:
        cpu->V[x] += kk;
        break;
      case 0x8:
        switch (n) {
          //LD Vx,  Vy
          case 0x0:
            cpu->V[x] = cpu->V[y];
            break;
          //OR Vx, Vy
          case 0x1:
            cpu->V[x] |= cpu->V[y];
            break;
          //AND Vx, Vy
          case 0x2:
            cpu->V[x] &= cpu->V[y];
            break;
          //XOR Vx, Vy
          case 0x3:
            cpu->V[x] ^= cpu->V[y];
            break;
          //ADD Vx, Vy
          case 0x4:
            cpu->V[0xF] = (uint16_t) cpu->V[x] + cpu->V[y] > UINT8_MAX;
            cpu->V[x] += cpu->V[y];
            break;
          //SUB Vx, Vy
          case 0x5:
            cpu->V[x] -= cpu->V[y];
            cpu->V[0xF] = cpu->V[x] > cpu->V[y];
            break;
          //SHR Vx
          case 0x6:
            cpu->V[0xF] = cpu->V[x] & 0x01; 
            cpu->V[x] >>= 1; 
            break;
          //SUBN Vx, Vy
          case 0x7:
            cpu->V[x] = cpu->V[y] - cpu->V[x];
            cpu->V[0xF] = cpu->V[y] > cpu->V[x];
            break;
          //SHL Vx 
          case 0xE:
            cpu->V[0xF] = (cpu->V[x] >> 7) & 0x01; 
            cpu->V[x] <<= 1; 
            break;
          default:
            break;
        }
      //SNE Vx, Vy
      case 0x9:
        if (cpu->V[x] != cpu->V[y]) {
          cpu->PC += 2;
        }
        break;
      //LD I, addr
      case 0xA:
        cpu->I = addr;
        break;
      //JP V0, addr
      case 0xB:
        cpu->PC = addr + cpu->V[0];
        break;
      //RND Vx, byte
      case 0xC:
        cpu->V[x] = (uint8_t) rand() & kk;
        break;
      //DRW Vx, Vy, n
      case 0xD:
        cpu->V[0xF] = 0;
        for (uint8_t h = 0; h < n; h++) {
          uint8_t sprite = cpu->ram[(cpu->I + h)];
          uint8_t posY = (cpu->V[y] + h) % 32;
          for (uint8_t w = 0; w < 8; w++) {
            uint8_t posX = (cpu->V[x] + w) % 64;
            uint8_t bit = (sprite >> (7 - w)) & 0x01;
            uint8_t* pixel = display->pixels + (posX + posY * 64);
            uint8_t xor = bit ^ (*pixel & 0xFF);
            if (!xor) {
              cpu->V[0xF] = 1;
              *pixel = 0;
            } else {
              *pixel = 0xFF;
            } 
          }
        }
        break;
      case 0xE:
      case 0xF:
        switch (kk) {
          case 0x07:
          case 0x0A:
          case 0x15:
          case 0x18:
          case 0x1E:
            cpu->I += cpu->V[x];
            break;
          case 0x29:
          case 0x33:
            cpu->ram[cpu->I] = (cpu->V[x] / 100);
            cpu->ram[cpu->I + 1] = (cpu->V[x] / 10) % 10;
            cpu->ram[cpu->I + 2] = cpu->V[x] % 10;
            break;
          case 0x55:
            for (uint8_t i = 0; i <= x; i++) {
              cpu->ram[cpu->I + i] = cpu->V[i];
            }
            break;
          case 0x65:
            for (uint8_t i = 0; i <= x; i++) {
              cpu->V[i] = cpu->ram[cpu->I + i];
            }
            break;
        }
        break;
      default:
        break;
    }
}
