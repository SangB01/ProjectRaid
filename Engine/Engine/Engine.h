#pragma once
#include <memory>
#include <Core/Core.h>


// 전반적인 게임 베이스 엔진
namespace Craft {

	class Level;
	class Input;
	class Renderer;

	class CRAFT_API Engine
	{
		struct Setting {
			float framerate = 60.0f;
			int width = 90;
			int height = 27;
		};

	public:
		Engine();
		virtual ~Engine();

		void Run();

		void Quit();

		// 레벨 추가 요청 함수
		template<typename T,
			typename = std::enable_if_t<std::is_base_of<Level, T>::value>>
			void AddNewLevel() {
			nextLevel = std::make_shared<T>();
		}

		static Engine& Get();

	protected:
		void ProcessInput();

		void OnInitialized();

		void BeginPlay();

		void Tick(float deltaTime);

		void Draw();

		void Shutdown();

		void LoadEngineSetting();

		void SavePreviousInputStates();

	protected:
		bool isQuit = false;

		Setting setting;

		static Engine* instance;
	
		// 메인 레벨
		std::shared_ptr<Level> mainLevel;

		// 추가 요청된 레벨
		std::shared_ptr<Level> nextLevel;

		// 입력시스템 변수
		std::unique_ptr<Input> input;

		//렌더러
		std::unique_ptr<Renderer> renderer;
	
	};

}