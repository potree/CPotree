
#pragma once

#include "unsuck/unsuck.hpp"
#include "Attributes.h"
#include "Node.h"

struct Writer{

	virtual void write(Node* node, shared_ptr<Points>, int64_t numAccepted, int64_t numRejected) = 0;

	virtual void close() = 0;

};

