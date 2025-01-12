#include <stdio.h>
#include <ogc/consol.h>

#include "menu.h"
#include "config.h"
#include "pad.h"
#include "video.h"

void DrawHeading(void)
{
	int conX, conY;
	CON_GetMetrics(&conX, &conY);

	// Draw a nice heading.
	puts("cdbackup mod " VERSION ", by thepikachugamer, modded by Larsenv");
	puts("Backup/restore/export your Wii Message Board data.");
	for (int i = 0; i < conX; i++)
		putchar(0xcd); // backup ahaha funny
}

void MainMenu(int argc; MainMenuItem argv[argc], int argc)
{

	int x = 0;
	while (true)
	{
		MainMenuItem *item = argv + x;

		clear();
		DrawHeading();
		for (MainMenuItem *i = argv; i < argv + argc; i++)
		{
			if (i == item)
				printf("%s>>  %s\x1b[40m\x1b[39m\n", i->highlight_str, i->name);
			else
				printf("    %s\n", i->name);
		}

		switch (wait_button(0))
		{
		case WPAD_BUTTON_UP:
		{
			if (x-- == 0)
				x = argc - 1;
			break;
		}

		case WPAD_BUTTON_DOWN:
		{
			if (++x == argc)
				x = 0;
			break;
		}

		case WPAD_BUTTON_A:
		{
			if (!item->action)
				return;

			clear();
			DrawHeading();
			item->action();

			puts("\nPress any button to continue...");
			wait_button(0);

			clear();
			break;
		}

		case WPAD_BUTTON_B:
		case WPAD_BUTTON_HOME:
		{
			return;
		}
		}
	}
}
