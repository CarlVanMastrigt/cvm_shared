/**
Copyright 2024 Carl van Mastrigt

This file is part of cvm_shared.

cvm_shared is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

cvm_shared is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with cvm_shared.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "cvm_shared.h"

#warning do we want a syetem to spin up new workers if/when a gate is stalled? (trylock or atomic checking only?)


void cvm_sync_task_happens_before_task(cvm_task * a, cvm_task * b)
{
    cvm_task_add_dependencies(b, 1);
    cvm_task_add_successor(a,(cvm_sync_primitive*)b);
}

void cvm_sync_task_happens_before_gate(cvm_task * a, cvm_gate * b)
{
    cvm_gate_add_dependencies(b, 1);
    cvm_task_add_successor(a,(cvm_sync_primitive*)b);
}
