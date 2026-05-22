
#pragma once

namespace tadmn::spectrum {

const void *getFactory(const char* factory_id);
bool clapInit(const char* p);
void clapDeinit();

}