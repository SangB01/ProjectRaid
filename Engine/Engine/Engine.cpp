#include "Engine.h"
#include <Level/Level.h>
#include <Input/Input.h>
#include <Render/Renderer.h>
#include <Physics/CollisionSystem.h>
#include <iostream>
#include <Windows.h>  
#include <cassert>


namespace Craft {
	Engine* Engine::instance = nullptr;

	Engine::Engine()
	{
		//instance 초기화
		assert(!instance);
		instance = this;

		// 엔진 설정 로드
		LoadEngineSetting();

		// 입력 객체 생성
		input = std::make_unique <Input>();

		// 렌더러 객체 생성
		renderer = std::make_unique<Renderer>(
			Vector2(setting.width, setting.height)
		);
	}
	Engine::~Engine()
	{
		instance = nullptr;
	}
	void Engine::Run()
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);

		// 현재 시간 읽기
		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);

		// 프레임 계산을 위한 변수
		int64_t current = counter.QuadPart;
		int64_t previous = current;

		// 고정 프레임으로 만들기 위한 값
		float oneFrameTime = 1.0f / setting.framerate;

		// 엔진 루프
		while (true) {
			// 종료 조건 처리
			if (isQuit) {
				break;
			}

			// 프레임 처리
			ProcessInput();

			// 프레임 시간 계산
			QueryPerformanceCounter(&counter);
			current = counter.QuadPart;

			float deltaTime =
				static_cast<float>(current - previous)
				/ static_cast<float>(frequency.QuadPart); 

			// 고정 프레임 처리.
			if (deltaTime >= oneFrameTime) {
				// 게임 이벤트 함수 호출
				OnInitialized();

				// 게임 이벤트의 초기화 함수 (1번만 호출)
				BeginPlay();

				// 게임 업데이트 (초당 호출)
				Tick(deltaTime);

				// 화면 그리기
				Draw();

				// 레벨 전환 처리
				if (nextLevel) {

					// 기존 레벨 정리
					if (mainLevel) {
						mainLevel.reset();
					}

					// 추가 요청된 레벨을 메인 레벨로 설정
					mainLevel = nextLevel;

					// 포인터 정리 - 포인터를 null로 정리
					nextLevel.reset();
				}

				// 추가/ 제거 요청된 액터 정리
				if (mainLevel) {
					mainLevel->ProcessAddAndDestroyActors();

					// 액터의 이전 상태 저장 처리
					mainLevel->SavePreviousActorStates();
				}
				// 입력 상태 저장
				SavePreviousInputStates();

				// 현재 시간을 이전 시간으로 저장
				previous = current;
			}

		}

		// 종료 처리 함수 호출
		Shutdown();
	}
	void Engine::Quit()
	{
		// 엔진 종료 플래그 설정.
		isQuit = true;
	}
	Engine& Engine::Get()
	{
		assert(instance); 
		return *instance;
	}
	void Engine::ProcessInput()
	{
		assert(input);
		if (!input) {
			return;
		}
		input->ProcessInput();
	}
	void Engine::OnInitialized()
	{
		// 레벨 초기화 처리
		// 예외처리
		if (!mainLevel || mainLevel->HasInitialized())
		{
			return;
		}

		// 초기화 이벤트 호출
		mainLevel->OnInitialized();
	}
	void Engine::BeginPlay()
	{
		if (!mainLevel) {
			return;
		}

		// 레벨에 이벤트 전달
		mainLevel->BeginPlay();
	}
	void Engine::Tick(float deltaTime)
	{
		if (!mainLevel) {
			return;
		}

		mainLevel->Tick(deltaTime);

		CollisionSystem collisionSystem;
		collisionSystem.ProcessCollision(mainLevel->actorList);
	}

	void Engine::Draw()
	{
		if (!mainLevel) {
			return;
		}

		mainLevel->Draw();

		//렌더러에 Draw 이벤트 호출
		if (!renderer) {
			return;
		}

		renderer->Draw();
	}
	
	void Engine::SavePreviousInputStates()
	{
		assert(input);
		if (!input) {
			return;
		}
		input->SavePreviousStates();
	}

	void Engine::Shutdown()
	{
	}
	void Engine::LoadEngineSetting()
	{
		// 파일 열기
		FILE* file = nullptr;
		fopen_s(&file, "../Config/Setting.txt", "rt");

		// 예외 처리
		if (!file) {
			std::cout << "Failed to open engine setting file \n";

			// 디버그 모드에서 강제 중단 시키는 기능
			__debugbreak();
			return;
		}

		// 데이터 읽어오기
		const int bufferSize = 2048;
		char buffer[bufferSize] = {};

		size_t readSize = fread(buffer, sizeof(char), bufferSize, file);

		// 값 저장을 위해 서식 해석 (파싱)
		// 문자열 자르기(Split)
		char* context = nullptr;
		char* token = nullptr;
		// 파일에서 읽은 전체 문자열을 개행(\n) 문자 기준으로 자르기
		token = strtok_s(buffer, "\n", &context);

		// 반복해서 자르기
		while (token) {
			// 공백 전까지 읽은 문자열을 저장할 변수
			char key[15] = {};

			// 포맷을 지정한 문자열 읽기
			// 공백 문자를 만나면 그 전까지 읽어서 저장
			sscanf_s(token, "%s", key, 15);

			// 키 값을 비교해서 값 설정
			if (strcmp(key, "framerate") == 0) {
				sscanf_s(token, "framerate = %f", &setting.framerate);
			}
			else if (strcmp(key, "width") == 0)
			{
				sscanf_s(token, "width = %d", &setting.width);
			}
			else if (strcmp(key, "height") == 0)
			{
				sscanf_s(token, "height = %d", &setting.height);
			}

			// 나머지 문자열 자르기 (개행 문자 기준)
			token = strtok_s(nullptr, "\n", &context);
		}

		// 파일 닫기
		fclose(file);
		file = nullptr;
	}

}