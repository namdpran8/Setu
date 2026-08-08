#include "StubRegistry.h"
#include "../utils/Logger.h"
#include "../ui/WindowManager.h"
#include "../ui/LayoutInflater.h"
#include "../dex/ResourceManager.h"
#include "Interpreter.h" // For recursive Interpreter execution

std::unordered_map<std::string, StubFunc> StubRegistry::stubs;
ResourceManager* StubRegistry::m_resManager = nullptr;
MultiDexManager* StubRegistry::m_multiDexManager = nullptr;
std::unordered_map<int, InterpreterObject*> StubRegistry::clickListeners;

void StubRegistry::init(ResourceManager* resManager, MultiDexManager* multiDexManager) {
    m_resManager = resManager;
    m_multiDexManager = multiDexManager;
    registerActivityStubs();
    registerViewStubs();
}

bool StubRegistry::isStubbed(const std::string& methodSignature) {
    if (stubs.find(methodSignature) != stubs.end()) return true;
    if (methodSignature == "Landroid/content/Intent;-><init>(Landroid/content/Context;Ljava/lang/Class;)V") return true;
    if (methodSignature.find("->startActivity(Landroid/content/Intent;)V") != std::string::npos) return true;
    if (methodSignature.find("->setContentView(I)V") != std::string::npos) return true;
    if (methodSignature.find("->findViewById(I)Landroid/view/View;") != std::string::npos) return true;
    if (methodSignature.find("Lkotlin/jvm/internal/Intrinsics;->checkNotNullParameter") != std::string::npos ||
        methodSignature.find("Lkotlin/jvm/internal/Intrinsics;->checkNotNullExpressionValue") != std::string::npos) return true;
        
    // Common basic SDK classes we don't want to fail on yet
    if (methodSignature.find("Ljava/lang/Object;") != std::string::npos) return true;
    if (methodSignature.find("Ljava/lang/Class;") != std::string::npos) return true;
    if (methodSignature.find("Ljava/lang/Thread;") != std::string::npos) return true;
    if (methodSignature.find("Ljava/lang/StringBuilder;") != std::string::npos) return true;
    if (methodSignature.find("Ljava/util/") != std::string::npos) return true;
    if (methodSignature.find("Landroid/content/Context;") != std::string::npos) return true;
    if (methodSignature.find("Landroid/content/SharedPreferences") != std::string::npos) return true;
    if (methodSignature.find("Landroid/app/Activity;->getWindow") != std::string::npos) return true;
    if (methodSignature.find("Landroid/view/Window;") != std::string::npos) return true;
    
    return false;
}

bool StubRegistry::invoke(const std::string& methodSignature, InterpreterState* state, const std::vector<Value>& args, Value* outReturn) {
    auto it = stubs.find(methodSignature);
    if (it != stubs.end()) {
        return it->second(state, args, outReturn);
    } else {
        // Intent and Context methods
        if (methodSignature == "Landroid/content/Intent;-><init>(Landroid/content/Context;Ljava/lang/Class;)V") {
            Logger::i("StubRegistry", "Executed: Intent.<init>(Context, Class)");
            if (args.size() >= 3 && args[0].type == ValueType::OBJECT && args[2].type == ValueType::OBJECT) {
                InterpreterObject* intentObj = (InterpreterObject*)args[0].obj;
                InterpreterObject* classObj = (InterpreterObject*)args[2].obj;
                if (intentObj && classObj) {
                    auto it = classObj->fields.find("targetClass");
                    if (it != classObj->fields.end() && it->second.type == ValueType::OBJECT) {
                        InterpreterObject* strObj = (InterpreterObject*)it->second.obj;
                        intentObj->fields["targetClass"] = Value::MakeObject(strObj);
                    }
                }
            }
            return false; // Return false means NO exception thrown
        }
        if (methodSignature.find("->startActivity(Landroid/content/Intent;)V") != std::string::npos) {
            Logger::i("StubRegistry", "Executed: startActivity(Intent)");
            if (args.size() >= 2 && args[1].type == ValueType::OBJECT) {
                InterpreterObject* intentObj = (InterpreterObject*)args[1].obj;
                if (intentObj) {
                    auto it = intentObj->fields.find("targetClass");
                    if (it != intentObj->fields.end() && it->second.type == ValueType::OBJECT) {
                        InterpreterObject* strObj = (InterpreterObject*)it->second.obj;
                        std::string targetClassName = strObj->className;
                        Logger::i("StubRegistry", "Starting new activity: " + targetClassName);
                        
                        // Clear the old window
                        WindowManager::clearWindow();
                        
                        // Launch the new activity
                        if (m_multiDexManager) {
                            Interpreter vm;
                            
                            // Execute <init>
                            auto [initBc, initDex] = m_multiDexManager->getMethodBytecode(targetClassName + "-><init>()V");
                            if (!initBc.bytecode.empty() && initDex) {
                                InterpreterObject* newActivity = new InterpreterObject();
                                newActivity->className = targetClassName;
                                std::vector<Value> initArgs = { Value::MakeObject(newActivity) };
                                vm.executeMethod(initBc.bytecode, initDex, m_multiDexManager, initArgs, initBc.registers_size, initBc.ins_size);
                                
                                // Execute onCreate
                                auto [onCreateBc, onCreateDex] = m_multiDexManager->getMethodBytecode(targetClassName + "->onCreate(Landroid/os/Bundle;)V");
                                if (!onCreateBc.bytecode.empty() && onCreateDex) {
                                    // this, Bundle (null)
                                    std::vector<Value> createArgs = { Value::MakeObject(newActivity), Value::MakeNull() }; 
                                    vm.executeMethod(onCreateBc.bytecode, onCreateDex, m_multiDexManager, createArgs, onCreateBc.registers_size, onCreateBc.ins_size);
                                } else {
                                    Logger::w("StubRegistry", "Could not find onCreate for " + targetClassName);
                                }
                            } else {
                                Logger::w("StubRegistry", "Could not find <init> for " + targetClassName);
                            }
                        }
                    }
                }
            }
            return false; // Return false means NO exception thrown
        }
        
        if (methodSignature.find("->setContentView(I)V") != std::string::npos) {
            if (args.size() >= 2 && args[1].type == ValueType::INT) {
                int layoutId = args[1].i;
                Logger::d("StubRegistry", "Executed: setContentView(layoutId=" + std::to_string(layoutId) + ")");
                
                if (m_resManager) {
                    auto layoutParser = m_resManager->getLayout(layoutId);
                    if (layoutParser) {
                        const AxmlNode* root = layoutParser->getRootNode();
                        if (root) {
                            Logger::i("StubRegistry", "Inflating layout with root tag: " + root->tag);
                            LayoutInflater::inflate(root, WindowManager::getMainWindow(), m_resManager);
                        }
                    }
                }
            }
            return false;
        }

        if (methodSignature.find("->findViewById(I)Landroid/view/View;") != std::string::npos) {
            if (args.size() >= 2 && args[1].type == ValueType::INT) {
                int id = args[1].i;
                Logger::d("StubRegistry", "Executed: findViewById(id=" + std::to_string(id) + ")");
                
                HWND childHwnd = GetDlgItem(WindowManager::getMainWindow(), id);
                if (childHwnd) {
                    InterpreterObject* viewObj = new InterpreterObject();
                    viewObj->className = "Landroid/view/View;";
                    viewObj->nativeHandle = childHwnd;
                    if (outReturn) *outReturn = Value::MakeObject(viewObj);
                    Logger::d("StubRegistry", "findViewById found matching HWND!");
                } else {
                    if (outReturn) *outReturn = Value::MakeNull();
                    Logger::w("StubRegistry", "findViewById could not find HWND for id " + std::to_string(id));
                }
            }
            return false;
        }

        if (methodSignature.find("Landroid/view/ViewGroup;->getChildCount()I") != std::string::npos) {
            if (outReturn) *outReturn = Value::MakeInt(1);
            return false;
        }
        
        if (methodSignature.find("Landroid/view/ViewGroup;->getChildAt(I)Landroid/view/View;") != std::string::npos) {
            if (outReturn) *outReturn = args.size() > 0 ? args[0] : Value::MakeNull();
            return false;
        }
        
        // Kotlin Intrinsics
        if (methodSignature.find("Lkotlin/jvm/internal/Intrinsics;->checkNotNullParameter") != std::string::npos ||
            methodSignature.find("Lkotlin/jvm/internal/Intrinsics;->checkNotNullExpressionValue") != std::string::npos) {
            return false; // Return false means NO exception thrown
        }
        
        // Handle common standard stubs returning null or simple values so it doesn't crash
        if (methodSignature.find("Ljava/lang/Object;->getClass") != std::string::npos) {
            if (outReturn) *outReturn = Value::MakeNull();
            return false;
        }
        if (methodSignature.find("Ljava/lang/Class;->getName") != std::string::npos) {
            if (outReturn) *outReturn = Value::MakeNull();
            return false;
        }
        if (methodSignature.find("Ljava/lang/Object;-><init>") != std::string::npos) {
            return false;
        }
        if (methodSignature.find("Ljava/lang/StringBuilder;->") != std::string::npos) {
            if (outReturn) {
                if (methodSignature.find("->toString") != std::string::npos) *outReturn = Value::MakeNull();
                else *outReturn = args.size() > 0 ? args[0] : Value::MakeNull();
            }
            return false;
        }
        if (methodSignature.find("Landroid/content/Context;->getPackageName") != std::string::npos) {
            if (outReturn) *outReturn = Value::MakeNull();
            return false;
        }
        if (methodSignature.find("Landroid/content/Context;->getSharedPreferences") != std::string::npos) {
            if (outReturn) *outReturn = Value::MakeNull();
            return false;
        }
        if (methodSignature.find("Landroid/content/SharedPreferences;->getInt") != std::string::npos) {
            if (outReturn) *outReturn = args.size() > 2 ? args[2] : Value::MakeInt(0);
            return false;
        }
        if (methodSignature.find("Landroid/content/SharedPreferences;->getBoolean") != std::string::npos) {
            if (outReturn) *outReturn = args.size() > 2 ? args[2] : Value::MakeInt(0);
            return false;
        }
        if (methodSignature.find("Landroid/content/SharedPreferences;->getString") != std::string::npos) {
            if (outReturn) *outReturn = args.size() > 2 ? args[2] : Value::MakeNull();
            return false;
        }
        if (methodSignature.find("Landroid/content/SharedPreferences$Editor;->") != std::string::npos) {
            if (outReturn && methodSignature.find("->commit") == std::string::npos && methodSignature.find("->apply") == std::string::npos) {
                *outReturn = args.size() > 0 ? args[0] : Value::MakeNull();
            }
            return false;
        }

        if (methodSignature.find("Ljava/lang/Thread;->") != std::string::npos) {
            if (outReturn) *outReturn = Value::MakeNull();
            return false;
        }
        if (methodSignature.find("Landroid/app/Activity;->getWindow") != std::string::npos) {
            if (outReturn) *outReturn = Value::MakeNull();
            return false;
        }
        if (methodSignature.find("Landroid/view/Window;->") != std::string::npos) {
            return false;
        }
        if (methodSignature.find("Ljava/util/") != std::string::npos) {
            if (methodSignature.find("hasNext") != std::string::npos) {
                if (outReturn) *outReturn = Value::MakeInt(0);
            } else {
                if (outReturn) *outReturn = Value::MakeNull();
            }
            return false;
        }

        if (methodSignature.find("Landroid/app/FragmentTransaction;->") != std::string::npos) {
            if (outReturn && methodSignature.find("->commit") == std::string::npos) {
                *outReturn = args.size() > 0 ? args[0] : Value::MakeNull();
            }
            return false;
        }

        if (methodSignature.find("Landroid/content/res/Resources;->getResourceName") != std::string::npos) {
            if (outReturn) *outReturn = Value::MakeNull();
            return false;
        }

        Logger::w("StubRegistry", "Unimplemented stub: " + methodSignature);
        // We do not throw an exception here just because it's a stub missing, 
        // we'll just return false (no exception thrown) and ignore it for now.
        return false;
    }
}

InterpreterObject* StubRegistry::getClickListener(int controlId) {
    auto it = clickListeners.find(controlId);
    if (it != clickListeners.end()) {
        return it->second;
    }
    return nullptr;
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
        
    // 2. Activity::getLayoutInflater
    stubs["Landroid/app/Activity;->getLayoutInflater()Landroid/view/LayoutInflater;"] = 
        [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
            Logger::d("StubRegistry", "Executed: Activity.getLayoutInflater()");
            if (outReturn) *outReturn = Value::MakeObject(new InterpreterObject());
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
        
    // 2.5 LayoutInflater::inflate
    stubs["Landroid/view/LayoutInflater;->inflate(ILandroid/view/ViewGroup;Z)Landroid/view/View;"] = 
        [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
            if (args.size() >= 2 && args[1].type == ValueType::INT) {
                int layoutId = args[1].i;
                Logger::d("StubRegistry", "Executed: LayoutInflater.inflate(layoutId=" + std::to_string(layoutId) + ")");
                if (m_resManager) {
                    auto layoutParser = m_resManager->getLayout(layoutId);
                    if (layoutParser) {
                        const AxmlNode* root = layoutParser->getRootNode();
                        if (root) {
                            Logger::i("StubRegistry", "Inflating layout with root tag: " + root->tag);
                            LayoutInflater::inflate(root, WindowManager::getMainWindow(), m_resManager);
                            
                            InterpreterObject* viewObj = new InterpreterObject();
                            viewObj->className = "Landroid/view/View;";
                            viewObj->nativeHandle = WindowManager::getMainWindow();
                            if (outReturn) *outReturn = Value::MakeObject(viewObj);
                        }
                    }
                }
            } else {
                if (outReturn) *outReturn = Value::MakeNull();
            }
            return false;
        };
        
    // 3. View::setOnClickListener
    auto setOnClickListenerStub = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (args.size() >= 2 && args[0].type == ValueType::OBJECT && args[1].type == ValueType::OBJECT) {
            InterpreterObject* viewObj = (InterpreterObject*)args[0].obj;
            HWND hwnd = viewObj ? (HWND)viewObj->nativeHandle : nullptr;
            InterpreterObject* listener = (InterpreterObject*)args[1].obj;
            
            int controlId = GetDlgCtrlID(hwnd);
            if (controlId != 0 && listener != nullptr) {
                clickListeners[controlId] = listener;
                Logger::i("StubRegistry", "Registered onClickListener for control ID: " + std::to_string(controlId));
            } else {
                Logger::w("StubRegistry", "Failed to register onClickListener: invalid control ID or null listener.");
            }
        } else {
            Logger::e("StubRegistry", "setOnClickListener called with invalid arguments.");
        }
        return false;
    };
    stubs["Landroid/view/View;->setOnClickListener(Landroid/view/View$OnClickListener;)V"] = setOnClickListenerStub;
    stubs["Landroid/widget/Button;->setOnClickListener(Landroid/view/View$OnClickListener;)V"] = setOnClickListenerStub;
    stubs["Landroid/widget/TextView;->setOnClickListener(Landroid/view/View$OnClickListener;)V"] = setOnClickListenerStub;

    auto emptyStub = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) *outReturn = Value::MakeNull();
        return false;
    };

    // Added missing stubs
    stubs["Landroid/app/Activity;->onCreate(Landroid/os/Bundle;)V"] = emptyStub;
    stubs["Landroid/app/Activity;->setTheme(I)V"] = emptyStub;
    stubs["Landroid/app/Fragment;-><init>()V"] = emptyStub;
    stubs["Landroid/os/Handler;->removeCallbacks(Ljava/lang/Runnable;)V"] = emptyStub;
    stubs["Landroid/view/View;->setTag(ILjava/lang/Object;)V"] = emptyStub;
    stubs["Lz1/g;->e(Ljava/lang/Object;Ljava/lang/String;)V"] = emptyStub;
    
    stubs["Ljava/lang/Enum;->compareTo(Ljava/lang/Enum;)I"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) *outReturn = Value::MakeInt(1);
        return false;
    };
    stubs["Ljava/lang/Object;->toString()Ljava/lang/String;"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) *outReturn = Value::MakeNull();
        return false;
    };
}
