
#pragma once

#include "unsuck/unsuck.hpp"
#include "Attributes.h"
#include "Node.h"

struct Writer{

	virtual void write(Attributes& inputAttributes, Node* node, shared_ptr<Buffer> data, int64_t numAccepted, int64_t numRejected) = 0;

	virtual void close() = 0;

};

