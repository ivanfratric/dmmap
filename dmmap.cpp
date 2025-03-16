#include <stdio.h>
#include <windows.h>
#include <tlhelp32.h>
#include <unordered_map>
#include <conio.h>

#define GAME_DM 0
#define GAME_CSB 1

int game;
char* gamestr;

class Map {
public:
	Map(int _level) {
		level = _level;
		Load();
	}
	void SetLocation(int x, int y, int orientation);
	void Resize(int oldx, int oldy, int newx, int newy);
	void MarkInteresting();

	int level;
	int maxx = 0;
	int maxy = 0;
	int curx = -1;
	int cury = -1;
	int orientation = -1;
	char* data = NULL;

	void Load();
	void Save();

	void Draw();
};

std::unordered_map<int, Map*> maps;

void Map::Draw() {
	printf("\x1b[1;1H");
	for (int y = 0; y < maxy; y++) {
		for (int x = 0; x < maxx; x++) {
			if (data[y * maxx + x] == '?') {

			}
			if ((curx == x) && (cury == y)) {
				printf("\x1b[7m");
				if (data[y * maxx + x] == '?') {
					printf("\x1b[33m");
				} else {
					printf("\x1b[32m");
				}
				switch (orientation) {
				case 0:
					printf(u8"↑");
					break;
				case 1:
					printf(u8"→");
					break;
				case 2:
					printf(u8"↓");
					break;
				case 3:
					printf(u8"←");
					break;
				default:
					printf("@");
					break;
				}
				printf("\x1b[0m");
			}	else if (data[y * maxx + x] == '#') {
				printf("\x1b[7m");
				printf(".");
				printf("\x1b[0m");
			} else if (data[y * maxx + x] == '?') {
				printf("\x1b[7m");
				printf("\x1b[31m");
				printf(".");
				printf("\x1b[0m");
			} else {
				printf(".");
			}
		}
		printf("\n");
	}
}

void Map::Save() {
	char namebuf[32];
	sprintf(namebuf, "%s_level_%d.txt", gamestr, level);
	FILE* fp = fopen(namebuf, "wb");
	if (!fp) {
		printf("Error saving %s\n", namebuf);
		return;
	}
	fwrite(&maxx, sizeof(int), 1, fp);
	fwrite(&maxy, sizeof(int), 1, fp);
	fwrite(data, maxx * maxy, 1, fp);
	fclose(fp);
}

void Map::Load() {
	char namebuf[32];
	sprintf(namebuf, "%s_level_%d.txt", gamestr, level);
	FILE* fp = fopen(namebuf, "rb");
	if (!fp) return;
	fread(&maxx, sizeof(int), 1, fp);
	fread(&maxy, sizeof(int), 1, fp);
	if (data) free(data);
	data = (char*)malloc(maxx * maxy);
	fread(data, maxx * maxy, 1, fp);
	fclose(fp);
}

void Map::Resize(int oldx, int oldy, int newx, int newy) {
	char* olddata = data;
	char* newdata = (char* )malloc(newx * newy);
	memset(newdata, '.', newx * newy);
	for (int y = 0; y < oldy; y++) {
		for (int x = 0; x < oldx; x++) {
			newdata[y * newx + x] = olddata[y * oldx + x];
		}
	}
	maxx = newx;
	maxy = newy;
	free(olddata);
	data = newdata;
	printf("\x1b[1;1H\x1b[2J");
}

void Map::SetLocation(int x, int y, int orientation) {
	if ((x + 2) > maxx) {
		Resize(maxx, maxy, x + 2, maxy);
	}
	if ((y + 2) > maxy) {
		Resize(maxx, maxy, maxx, y + 2);
	}
	curx = x;
	cury = y;
	this->orientation = orientation;
	if (data[y * maxx + x] != '.') return;
	data[y * maxx + x] = '#';
	Save();
}

void Map::MarkInteresting() {
	char* pos = &data[cury * maxx + curx];
	if (*pos == '?') *pos = '#';
	else *pos = '?';
	Save();
}


int pid_from_name(char *name) {
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			if (stricmp(entry.szExeFile, name) == 0)
			{
				CloseHandle(snapshot);
				return entry.th32ProcessID;
			}
		}
	}
	CloseHandle(snapshot);
	return 0;
}

size_t finddmsave(size_t addr, char* data, size_t size) {
	if (game == GAME_DM) {
		for (size_t i = 0; i < (size - 216); i++) {
			if ((data[i] == 'D') &&
				(data[i + 1] == 'F') &&
				(data[i + 2] == '0') &&
				(data[i + 3] == ':'))
			{
				if (!memcmp(data + i + 6, "DungeonMaster:", 14)) {
					if (!memcmp(data + i + 22, "DungeonSave:", 12)) {
						return addr + i;
					}
				}
			}
		}
	} else if (game == GAME_CSB) {
		for (size_t i = 0; i < (size - 216); i++) {
			if ((data[i] == 'D') &&
				(data[i + 1] == 'F') &&
				(data[i + 2] == '0') &&
				(data[i + 3] == ':') &&
				(data[i + 6] == 'D') &&
				(data[i + 7] == 'F') &&
				(data[i + 8] == '1') &&
				(data[i + 9] == ':'))
			{
				if (!memcmp(data + i + 12, "DungeonMaster:", 14)) {
					if (!memcmp(data + i + 28, "DungeonSave:", 12)) {
						return addr + i;
					}
				}
			}
		}
	}
	return 0;
}

size_t finddmsave(HANDLE proc) {
	MEMORY_BASIC_INFORMATION meminfobuf;
	size_t address = 0;

	DWORD readflags = PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READWRITE |
		PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY;

	while (1) {
		size_t ret = VirtualQueryEx(proc, (LPCVOID)address, &meminfobuf, sizeof(MEMORY_BASIC_INFORMATION));
		if (!ret) break;

		if ((meminfobuf.State & MEM_COMMIT) && (meminfobuf.Protect & readflags) && !(meminfobuf.Protect & PAGE_GUARD)) {
			if (meminfobuf.RegionSize >= (512 * 1024)) {
				char *data = (char*)malloc(meminfobuf.RegionSize);
				size_t numbytesread;
				if (ReadProcessMemory(proc, (LPCVOID)meminfobuf.BaseAddress, (LPVOID)(data), meminfobuf.RegionSize, &numbytesread)) {
					size_t addr = finddmsave((size_t)meminfobuf.BaseAddress, data, (size_t)meminfobuf.RegionSize);
					free(data);
					if (addr) return addr;
				}
			}
		}

		address = (size_t)meminfobuf.BaseAddress + meminfobuf.RegionSize;
	}

	return 0;
}

int getlocation(HANDLE proc, size_t dmsave_addr, unsigned char* posx, unsigned char* posy, unsigned char* level, unsigned char* orientation) {
	char buf[196];
	size_t numbytesread;
	if (!ReadProcessMemory(proc, (LPCVOID)dmsave_addr, buf, sizeof(buf), &numbytesread)) {
		return 0;
	}
	if (numbytesread != sizeof(buf)) return 0;

	if (game == GAME_DM) {
		if ((buf[0] == 'D') &&
			(buf[1] == 'F') &&
			(buf[2] == '0') &&
			(buf[3] == ':'))
		{
			if (!memcmp(buf + 6, "DungeonMaster:", 14)) {
				if (!memcmp(buf + 22, "DungeonSave:", 12)) {
					*posx = buf[191];
					*posy = buf[193];
					*level = buf[195];
					*orientation = buf[189];
					return 1;
				}
			}
		}
	} else if (game == GAME_CSB) {
		if ((buf[0] == 'D') &&
			(buf[1] == 'F') &&
			(buf[2] == '0') &&
			(buf[3] == ':') &&
			(buf[6] == 'D') &&
			(buf[7] == 'F') &&
			(buf[8] == '1') &&
			(buf[9] == ':'))
		{
			if (!memcmp(buf + 12, "DungeonMaster:", 14)) {
				if (!memcmp(buf + 28, "DungeonSave:", 12)) {
					*posx = buf[97];
					*posy = buf[99];
					*orientation = buf[101];
					*level = buf[103];
					return 1;
				}
			}
		}
	}
	return 0;
}

CRITICAL_SECTION cs;
Map* current_map;

HHOOK _hook;
LRESULT HookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
	//printf("hook\n");
	KBDLLHOOKSTRUCT kbdStruct;
	if (nCode >= 0) {
		if (wParam == WM_KEYDOWN) {
			kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);
			if (kbdStruct.vkCode == 0x58) {
				EnterCriticalSection(&cs);
				if (current_map) {
					current_map->MarkInteresting();
				}
				LeaveCriticalSection(&cs);
			}
		}
	}

	return CallNextHookEx(_hook, nCode, wParam, lParam);
}

DWORD WINAPI KeyboardHookThread(LPVOID _lpParam) {
	if (!(_hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallback, NULL, 0)))
	{
		printf("SetWindowsHookEx error\n");
		return 0;
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) { }
	return 0;
}

#define N_PREVIOUS_LEVELS 6

int main(int argc, char** argv) {
	SetConsoleOutputCP(CP_UTF8);

	if (argc < 2) {
		printf("Usage: %s <dm|csb> <process name>\n", argv[0]);
		printf("e.g.: %s dm retroarch.exe\n", argv[0]);
		printf("e.g.: %s dm fs-uae.exe\n", argv[0]);
		return 0;
	}

	gamestr = argv[1];
	if (!strcmp(gamestr, "dm")) {
		game = GAME_DM;
	} else if (!strcmp(gamestr, "csb")) {
		game = GAME_CSB;
	} else {
		printf("Unknown game id param %s, must be dm or csb", gamestr);
		return 0;
	}

	int pid = pid_from_name(argv[2]);
	if (!pid) {
		printf("Error finding process %s\n", argv[2]);
		return 0;
	}

	HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE, false, pid);
	if (!proc) {
		printf("Error opening process\n");
		return 0;
	}

	unsigned char posx, posy, level, orientation, previous_level = 0xFF;
	unsigned char previous_levels[N_PREVIOUS_LEVELS];
	memset(previous_levels, 0xFF, N_PREVIOUS_LEVELS);

	// Set output mode to handle virtual terminal sequences
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
	{
		printf("GetStdHandle error\n");
		return 0;
	}
	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode))
	{
		printf("GetConsoleMode error\n");
		return 0;
	}
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode))
	{
		printf("SetConsoleMode error\n");
		return 0;
	}

	InitializeCriticalSection(&cs);
	CreateThread(NULL, 0, KeyboardHookThread, NULL, 0, NULL);

	while (true) {
		size_t addr = finddmsave(proc);
		if (!addr) {
			DWORD wait_ret = WaitForSingleObject(proc, 0);
			if (wait_ret != WAIT_TIMEOUT) {
				printf("Game exited, will also exit\n");
				break;
			}
			printf("Couldn't find location, level not loaded or unsupported version\n");
			Sleep(1000);
			continue;
		}
		printf("dmsave addr: %zx\n", addr);
		printf("\x1b[1;1H\x1b[2J");
		printf("\x1b[?25l");
		while (true) {
			if (!getlocation(proc, addr, &posx, &posy, &level, &orientation)) {
				printf("\x1b[1;1H\x1b[2J");
				printf("Couldn't find location, level not loaded or unsupported version\n");
				EnterCriticalSection(&cs);
				current_map = NULL;
				LeaveCriticalSection(&cs);
				memset(previous_levels, 0xFF, N_PREVIOUS_LEVELS);
				Sleep(1000);
				break;
			}
			printf("x:%d  y:%d  level:%d      \n", posx, posy, level); // spaces deliberate

			if ((!level && !posx && !posy && !current_map) ||
				(level == 0xFF))
			{
				EnterCriticalSection(&cs);
				current_map = NULL;
				LeaveCriticalSection(&cs);
				memset(previous_levels, 0xFF, N_PREVIOUS_LEVELS);
				Sleep(1000);
				continue;
			}

			// during game start / loading, level changes quickly
			// wait a bit until it settles
			if (!current_map) {
				bool settled = true;
				for (int i = 0; i < N_PREVIOUS_LEVELS; i++) {
					if (level != previous_levels[i]) {
						settled = false;
						break;
					}
				}
				if (!settled) {
					for (int i = 0; i < (N_PREVIOUS_LEVELS - 1); i++) {
						previous_levels[i] = previous_levels[i + 1];
					}
					previous_levels[N_PREVIOUS_LEVELS - 1] = level;
					Sleep(500);
					continue;
				}
			}

			EnterCriticalSection(&cs);
			if (!current_map || (current_map->level != level)) {
				std::unordered_map<int, Map*>::iterator iter = maps.find(level);
				if (iter == maps.end()) {
					current_map = new Map(level);
					maps[level] = current_map;
				} else {
					current_map = iter->second;
				}
				printf("\x1b[1;1H\x1b[2J");
			}

			current_map->SetLocation(posx + 1, posy + 1, orientation);
			current_map->Draw();

			LeaveCriticalSection(&cs);

			Sleep(100);
		}
	}

	return 0;
}
