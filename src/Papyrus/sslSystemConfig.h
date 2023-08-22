#pragma once

namespace Papyrus::SystemConfig
{
	int GetSettingInt(VM* a_vm, StackID a_stackID, RE::StaticFunctionTag*, std::string a_setting);
	float GetSettingFlt(VM* a_vm, StackID a_stackID, RE::StaticFunctionTag*, std::string a_setting);
	bool GetSettingBool(VM* a_vm, StackID a_stackID, RE::StaticFunctionTag*, std::string a_setting);
	int GetSettingIntA(VM* a_vm, StackID a_stackID, RE::StaticFunctionTag*, std::string a_setting, size_t n);
	float GetSettingFltA(VM* a_vm, StackID a_stackID, RE::StaticFunctionTag*, std::string a_setting, size_t n);

	void SetSettingInt(VM* a_vm, StackID a_stackID, RE::StaticFunctionTag*, std::string a_setting, int a_value);
	void SetSettingFlt(VM* a_vm, StackID a_stackID, RE::StaticFunctionTag*, std::string a_setting, float a_value);
	void SetSettingBool(VM* a_vm, StackID a_stackID, RE::StaticFunctionTag*, std::string a_setting, bool a_value);
	void SetSettingIntA(VM* a_vm, StackID a_stackID, RE::StaticFunctionTag*, std::string a_setting, int a_value, int n);
	void SetSettingFltA(VM* a_vm, StackID a_stackID, RE::StaticFunctionTag*, std::string a_setting, float a_value, int n);

	inline bool Register(VM* a_vm)
	{
		REGISTERFUNC(GetSettingInt, "sslSystemConfig", true);
		REGISTERFUNC(GetSettingFlt, "sslSystemConfig", true);
		REGISTERFUNC(GetSettingBool, "sslSystemConfig", true);
		REGISTERFUNC(GetSettingIntA, "sslSystemConfig", true);
		REGISTERFUNC(GetSettingFltA, "sslSystemConfig", true);

		REGISTERFUNC(SetSettingInt, "sslSystemConfig", true);
		REGISTERFUNC(SetSettingFlt, "sslSystemConfig", true);
		REGISTERFUNC(SetSettingBool, "sslSystemConfig", true);
		REGISTERFUNC(SetSettingIntA, "sslSystemConfig", true);
		REGISTERFUNC(SetSettingFltA, "sslSystemConfig", true);

		return true;
	}

} // namespace Papyrus
