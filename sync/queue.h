/**
Copyright 2025 Carl van Mastrigt

This file is part of solipsix.

solipsix is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

solipsix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with solipsix.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "cvm_sync.h"

#ifndef CVM_SYNC_QUEUE_H
#define CVM_SYNC_QUEUE_H

struct cvm_sync_queue
{
    union cvm_sync_primitive* _Atomic end_primitive;
};

void cvm_sync_queue_initialise(struct cvm_sync_queue* queue);
void cvm_sync_queue_terminate(struct cvm_sync_queue* queue);

void cvm_sync_queue_enqueue_primitive(struct cvm_sync_queue* queue, union cvm_sync_primitive* primitive);
void cvm_sync_queue_enqueue_primitive_range(struct cvm_sync_queue* queue, union cvm_sync_primitive* first_primitive, union cvm_sync_primitive* last_primitive);

#endif
