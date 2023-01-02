#pragma once

#define FIELD_OFFSET(type, field)    ((int)(intptr_t)&(((type *)0)->field))
