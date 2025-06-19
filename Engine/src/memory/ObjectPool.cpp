#include "../include/memory/ObjectPool.h"
#include "../include/components/Component.h"
#include <iostream>
#include <typeindex>

// Static instance initialization
PoolManager* PoolManager::instance = nullptr;