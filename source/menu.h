typedef const struct
{
    const char* name;
    const char* highlight_str;
    int (*action)(void);
} MainMenuItem;

void DrawHeading(void);
void MainMenu(int argc; MainMenuItem argv[argc], int argc);
