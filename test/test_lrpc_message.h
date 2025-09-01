#pragma once
#include <stdint.h>


// Simple request and response structures
struct AddRequest
{
    uint64_t a;
    uint64_t b;
};

struct AddResponse
{
    uint64_t result;
};
