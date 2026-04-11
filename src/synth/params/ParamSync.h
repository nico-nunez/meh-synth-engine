#pragma once

#include "synth/params/ParamDefs.h"

namespace synth {
struct Engine;
}

namespace synth::param::sync {

void initParamDefaults(Engine& engine);

void setParam(Engine& engine, ParamID id, float value);
void setParamDeferred(Engine& engine, ParamID id, float value);

void syncParam(Engine& engine, ParamID id);
void syncDirtyParams(Engine& eng);
void syncAllParams(Engine& engine);

} // namespace synth::param::sync
