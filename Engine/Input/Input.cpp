#include "Input.h"
#include <cassert>
#include <Windows.h>

namespace Craft {
	Input* Input::instance = nullptr;

	Input::Input()
	{
		assert(!instance);
		instance = this;
	}

	bool Input::GetKeyDown(int keyCode) const
	{

		return !keyStates[keyCode].wasKeyDown
			&& keyStates[keyCode].isKeyDown;
	}

	bool Input::GetKeyUp(int keyCode) const
	{
		return keyStates[keyCode].wasKeyDown
			&& !keyStates[keyCode].isKeyDown;
	}

	bool Input::GetKey(int keyCode) const
	{
		return keyStates[keyCode].isKeyDown;
	}

	Input& Input::Get()
	{
		assert(instance);
		return *instance;
	}

	void Input::ProcessInput()
	{
		// 현재 프레임에 키 입력이 발생하였는지 확인
		for (int ix = 0; ix < keyCount; ++ix) {
			keyStates[ix].isKeyDown = ((GetAsyncKeyState(ix) & 0x8000) != 0);
		}
	}
	void Input::SavePreviousStates()
	{
		// 이전 프레임 입력 값 저장
		for (KeyState& state : keyStates) {
			// 현재 프레임 입력 값을 이전 프레임 값으로 저장
			state.wasKeyDown = state.isKeyDown;
		}
	}
}