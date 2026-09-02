#pragma once

#include "stddef.h"

#define alignof(type) offsetof(struct { char c; type field; }, field)
