#include "StubRegistry.h"
#include "../utils/Logger.h"
#include "../ui/WindowManager.h"

std::unordered_map<std::string, StubFunc> StubRegistry::stubs;

void StubRegistry::init() {
    registerActivityStubs();
    registerViewStubs();
}

bool StubRegistry::invoke(const std::string& methodSignature, InterpreterState* state, const std::vector<Value>& args, Value* outReturn) {
    auto it = stubs.find(methodSignature);
    if (it != stubs.end()) {
        return it->second(state, args, outReturn);
    } else {
        Logger::w("StubRegistry", "Unimplemented stub: " + methodSignature);
        // We do not throw an exception here just because it's a stub missing, 
        // we'll just return false (no exception thrown) and ignore it for now.
        return false; 
    }
}

void StubRegistry::registerActivityStubs() {
    // 1. AppCompatActivity::onCreate
    stubs["Landroidx/appcompat/app/AppCompatActivity;->onCreate(Landroid/os/Bundle;)V"] = 
        [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
            Logger::d("StubRegistry", "Executed: AppCompatActivity.onCreate() -> Showing Window");
            HWND mainWnd = WindowManager::getMainWindow();
            if (mainWnd) {
                ShowWindow(mainWnd, SW_SHOW);
                UpdateWindow(mainWnd);
            }
            return false;
        };

    // 2. MainActivity::setContentView(I)V
    stubs["Lcom/pranshu/test1/MainActivity;->setContentView(I)V"] = 
        [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
            if (args.size() >= 2 && args[1].type == ValueType::INT) {
                Logger::d("StubRegistry", "Executed: MainActivity.setContentView(layoutId=" + std::to_string(args[1].i) + ")");
                WindowManager::createStaticText("Windroid - setContentView called!", 50, 50, 300, 50);
            } else {
                Logger::e("StubRegistry", "setContentView called with invalid arguments!");
            }
            return false;
        };

    // 3. MainActivity::findViewById(I)Landroid/view/View;
    stubs["Lcom/pranshu/test1/MainActivity;->findViewById(I)Landroid/view/View;"] = 
        [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
            if (args.size() >= 2 && args[1].type == ValueType::INT) {
                int layoutId = args[1].i;
                Logger::d("StubRegistry", "Executed: MainActivity.findViewById(id=" + std::to_string(layoutId) + ")");
                
                MockView* dummyView = new MockView();
                dummyView->debugTag = "view_id_" + std::to_string(layoutId);
                
                *outReturn = Value::MakeObject(dummyView);
            }
            return false;
        };
}

void StubRegistry::registerViewStubs() {
    // 1. EdgeToEdge::enable(Landroidx/activity/ComponentActivity;)V
    stubs["Landroidx/activity/EdgeToEdge;->enable(Landroidx/activity/ComponentActivity;)V"] = 
        [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
            Logger::d("StubRegistry", "Executed: EdgeToEdge.enable() (No-op)");
            return false;
        };

    // 2. ViewCompat::setOnApplyWindowInsetsListener(Landroid/view/View;Landroidx/core/view/OnApplyWindowInsetsListener;)V
    stubs["Landroidx/core/view/ViewCompat;->setOnApplyWindowInsetsListener(Landroid/view/View;Landroidx/core/view/OnApplyWindowInsetsListener;)V"] = 
        [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
            Logger::d("StubRegistry", "Executed: ViewCompat.setOnApplyWindowInsetsListener() (No-op)");
            return false;
        };
}
