#ifndef WINDOW_H
#define WINDOW_H

bool window_init_default(void);
bool window_init(unsigned int w, unsigned int h);
bool window_loop(void);

float window_get_aspect_ratio(void);

#endif /* WINDOW_H */