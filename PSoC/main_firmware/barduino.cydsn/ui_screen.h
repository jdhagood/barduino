#ifndef UI_SCREEN_H
#define UI_SCREEN_H

typedef struct
{
    void (*on_enter)(void);
    void (*on_exit)(void);
    void (*update)(void);
    void (*draw)(void);
} ui_screen_t;

#endif