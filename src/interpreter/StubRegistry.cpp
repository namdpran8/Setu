#include "StubRegistry.h"
#include "../utils/Logger.h"
#include "../ui/WindowManager.h"
#include "../ui/LayoutInflater.h"
#include "../dex/ResourceManager.h"
#include "../view/ViewGroup.h"
#include "../widget/TextView.h"
#include "Interpreter.h" // For recursive Interpreter execution

std::unordered_map<std::string, StubFunc> StubRegistry::stubs;
ResourceManager* StubRegistry::m_resManager = nullptr;
MultiDexManager* StubRegistry::m_multiDexManager = nullptr;
std::unordered_map<int, InterpreterObject*> StubRegistry::clickListeners;

static std::wstring utf8_to_utf16(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &utf8[0], (int)utf8.size(), NULL, 0);
    std::wstring utf16(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &utf8[0], (int)utf8.size(), &utf16[0], size_needed);
    return utf16;
}

static std::string utf16_to_utf8(const std::wstring& utf16) {
    if (utf16.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &utf16[0], (int)utf16.size(), NULL, 0, NULL, NULL);
    std::string utf8(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &utf16[0], (int)utf16.size(), &utf8[0], size_needed, NULL, NULL);
    return utf8;
}

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
    if (methodSignature.find("Landroidx/fragment/app/FragmentManager") != std::string::npos) return true;
    if (methodSignature.find("Landroidx/fragment/app/FragmentTransaction") != std::string::npos) return true;
    if (methodSignature.find("Landroidx/fragment/app/FragmentActivity;->getSupportFragmentManager") != std::string::npos) return true;
    if (methodSignature.find("Landroidx/lifecycle/") != std::string::npos) return true;
    if (methodSignature.find("Ljava/lang/Enum;") != std::string::npos) return true;
    if (methodSignature.find("Ljava/lang/Integer;") != std::string::npos) return true;
    if (methodSignature.find("Landroid/graphics/Rect;") != std::string::npos) return true;
    if (methodSignature.find("Landroid/animation/LayoutTransition;") != std::string::npos) return true;
    if (methodSignature.find("Ljava/lang/IllegalStateException;") != std::string::npos) return true;
    if (methodSignature.find("Landroid/widget/OverScroller;") != std::string::npos) return true;
    if (methodSignature.find("Landroid/widget/ImageView;") != std::string::npos) return true;
    if (methodSignature.find("Landroid/view/View;") != std::string::npos) return true;
    if (methodSignature.find("Landroid/view/ViewGroup;") != std::string::npos) return true;
    if (methodSignature.find("Landroid/widget/TextView;") != std::string::npos) return true;
    if (methodSignature.find("Lj0/w;") != std::string::npos) return true;
    if (methodSignature.find("Lz1/g;") != std::string::npos) return true;
    if (methodSignature.find("Ln0/a;") != std::string::npos) return true;
    if (methodSignature.find("Lu0/c;") != std::string::npos) return true;
    if (methodSignature.find("Lg/F;") != std::string::npos) return true;
    if (methodSignature.find("Landroid/os/Build$VERSION;") != std::string::npos) return true;
    if (methodSignature.find("Lx0/b;") != std::string::npos) return true;
    if (methodSignature.find("Landroidx/recyclerview/") != std::string::npos) return true;
    if (methodSignature.find("Landroid/util/SparseArray;") != std::string::npos) return true;
    if (methodSignature.find("Ljava/lang/ThreadLocal;") != std::string::npos) return true;
    
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
                        WindowManager::clearWindow(); // Restored temporarily until Phase 2 is complete
                        
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
                            RECT rect;
                            GetClientRect(WindowManager::getMainWindow(), &rect);
                            int pW = rect.right > 0 ? rect.right : 400;
                            int pH = rect.bottom > 0 ? rect.bottom : 800;
                            auto rootView = windroid::LayoutInflater::inflate(root, m_resManager);
                            WindowManager::setRootView(rootView);
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
                
                windroid::view::View* childView = nullptr;
                std::function<windroid::view::View*(windroid::view::View*, int)> findViewRecursive = [&](windroid::view::View* parent, int searchId) -> windroid::view::View* {
                    if (!parent) return nullptr;
                    if (parent->getId() == searchId) return parent;
                    
                    auto viewGroup = dynamic_cast<windroid::view::ViewGroup*>(parent);
                    if (viewGroup) {
                        for (size_t i = 0; i < viewGroup->getChildCount(); ++i) {
                            auto found = findViewRecursive(viewGroup->getChildAt(i).get(), searchId);
                            if (found) return found;
                        }
                    }
                    return nullptr;
                };
                
                childView = findViewRecursive(WindowManager::getRootView().get(), id);
                
                if (childView) {
                    InterpreterObject* viewObj = new InterpreterObject();
                    viewObj->className = "Landroid/view/View;";
                    viewObj->nativeHandle = childView;
                    if (outReturn) *outReturn = Value::MakeObject(viewObj);
                    Logger::d("StubRegistry", "findViewById found matching View!");
                } else {
                    if (outReturn) *outReturn = Value::MakeNull();
                    Logger::w("StubRegistry", "findViewById could not find View for id " + std::to_string(id));
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

        if (methodSignature.find("Landroid/app/FragmentTransaction;->") != std::string::npos ||
            methodSignature.find("Landroidx/fragment/app/FragmentTransaction;->") != std::string::npos) {
            if (outReturn && methodSignature.find("->commit") == std::string::npos) {
                *outReturn = args.size() > 0 ? args[0] : Value::MakeNull();
            }
            return false;
        }

        if (methodSignature.find("Landroidx/fragment/app/FragmentManager;->") != std::string::npos ||
            methodSignature.find("Landroid/app/FragmentManager;->") != std::string::npos) {
            if (outReturn) {
                if (methodSignature.find("->beginTransaction") != std::string::npos) {
                    *outReturn = Value::MakeObject(new InterpreterObject()); // Dummy transaction
                } else {
                    *outReturn = Value::MakeNull();
                }
            }
            return false;
        }

        if (methodSignature.find("Landroidx/lifecycle/") != std::string::npos) {
            if (outReturn) *outReturn = Value::MakeNull();
            return false;
        }

        if (methodSignature.find("Landroidx/fragment/app/FragmentActivity;->getSupportFragmentManager") != std::string::npos ||
            methodSignature.find("Landroid/app/Activity;->getFragmentManager") != std::string::npos) {
            if (outReturn) *outReturn = Value::MakeObject(new InterpreterObject());
            return false;
        }

        if (methodSignature.find("Landroid/content/res/Resources;->getResourceName") != std::string::npos) {
            if (outReturn) *outReturn = Value::MakeNull();
            return false;
        }

        Logger::w("StubRegistry", "Unimplemented stub: " + methodSignature);
        // We do not throw an exception here just because it's a stub missing, 
        // we'll just return false (no exception thrown) and ignore it for now.
        if (outReturn) *outReturn = Value::MakeNull();
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
    static std::vector<std::shared_ptr<windroid::view::View>> s_dynamicViews;
    
    // Basic dynamic view creation helper
    auto dynamicViewInit = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn, const std::string& className) -> bool {
        if (args.size() > 0 && args[0].type == ValueType::OBJECT && args[0].obj) {
            InterpreterObject* viewObj = (InterpreterObject*)args[0].obj;
            auto view = std::make_shared<windroid::view::ViewGroup>(); // generic for now
            s_dynamicViews.push_back(view);
            viewObj->nativeHandle = view.get();
            Logger::d("StubRegistry", "Created dynamic view: " + className);
        }
        return false;
    };

    stubs["Landroid/widget/HorizontalScrollView;-><init>(Landroid/content/Context;)V"] = 
        [dynamicViewInit](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
            return dynamicViewInit(state, args, outReturn, "Landroid/widget/HorizontalScrollView;");
        };
    stubs["Landroid/widget/HorizontalScrollView;-><init>(Landroid/content/Context;Landroid/util/AttributeSet;)V"] = 
        [dynamicViewInit](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
            return dynamicViewInit(state, args, outReturn, "Landroid/widget/HorizontalScrollView;");
        };
    stubs["Landroid/view/View;-><init>(Landroid/content/Context;)V"] = 
        [dynamicViewInit](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
            return dynamicViewInit(state, args, outReturn, "Landroid/view/View;");
        };
        
    stubs["Landroid/widget/HorizontalScrollView;->setFillViewport(Z)V"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool { return false; };

    stubs["Landroid/view/View;->setId(I)V"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (args.size() >= 2 && args[0].type == ValueType::OBJECT && args[0].obj && args[1].type == ValueType::INT) {
            InterpreterObject* viewObj = (InterpreterObject*)args[0].obj;
            windroid::view::View* view = (windroid::view::View*)viewObj->nativeHandle;
            int id = args[1].i;
            if (view) {
                view->setId(id);
                Logger::d("StubRegistry", "Executed View.setId(" + std::to_string(id) + ")");
            }
        }
        return false;
    };
    
    stubs["Landroid/view/View;->setVisibility(I)V"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        return false; // Skip visibility for now
    };

    stubs["Landroid/view/ViewGroup;->addView(Landroid/view/View;II)V"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        return false; // Dynamic view insertion skipped to prevent crashes for now
    };

    stubs["Landroid/view/ViewGroup;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V"] = 
        stubs["Landroid/view/ViewGroup;->addView(Landroid/view/View;II)V"];

    stubs["Landroid/view/ViewGroup;->removeView(Landroid/view/View;)V"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        return false;
    };

    stubs["Landroid/view/ViewGroup;->removeAllViews()V"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        return false;
    };

    stubs["Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V"] = 
        [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
            if (args.size() >= 2 && args[0].type == ValueType::OBJECT && args[0].obj && args[1].type == ValueType::OBJECT && args[1].obj) {
                InterpreterObject* viewObj = (InterpreterObject*)args[0].obj;
                windroid::widget::TextView* view = (windroid::widget::TextView*)viewObj->nativeHandle;
                InterpreterObject* strObj = (InterpreterObject*)args[1].obj;
                
                std::string strVal;
                if (strObj->className == "Ljava/lang/String;" && strObj->fields.count("string_value")) {
                    strVal = ((InterpreterObject*)strObj->fields["string_value"].obj)->className;
                } else {
                    strVal = strObj->className; // Fallback to class name if it's a raw string wrapper
                }
                
                if (view && !strVal.empty()) {
                    view->setText(utf8_to_utf16(strVal));
                    Logger::d("StubRegistry", "TextView.setText: " + strVal);
                }
            }
            return false;
        };
        
    stubs["Landroid/widget/TextView;->getText()Ljava/lang/CharSequence;"] = 
        [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
            if (args.size() >= 1 && args[0].type == ValueType::OBJECT && args[0].obj) {
                InterpreterObject* viewObj = (InterpreterObject*)args[0].obj;
                windroid::widget::TextView* view = (windroid::widget::TextView*)viewObj->nativeHandle;
                if (view) {
                    std::string strVal = utf16_to_utf8(view->getText());
                    InterpreterObject* strObj = new InterpreterObject();
                    strObj->className = "Ljava/lang/String;";
                    InterpreterObject* inner = new InterpreterObject();
                    inner->className = strVal;
                    strObj->fields["string_value"] = Value::MakeObject(inner);
                    if (outReturn) *outReturn = Value::MakeObject(strObj);
                    return false;
                }
            }
            if (outReturn) *outReturn = Value::MakeNull();
            return false;
        };

    stubs["Landroid/widget/RelativeLayout$LayoutParams;-><init>(II)V"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool { return false; };
    stubs["Landroid/widget/RelativeLayout$LayoutParams;->addRule(II)V"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool { return false; };

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
                            RECT rect;
                            GetClientRect(WindowManager::getMainWindow(), &rect);
                            int pW = rect.right > 0 ? rect.right : 400;
                            int pH = rect.bottom > 0 ? rect.bottom : 800;
                            auto rootView = windroid::LayoutInflater::inflate(root, m_resManager);
                            WindowManager::setRootView(rootView);
                            
                            InterpreterObject* viewObj = new InterpreterObject();
                            viewObj->className = "Landroid/view/View;";
                            viewObj->nativeHandle = rootView.get();
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
        if (args.size() >= 2 && args[0].type == ValueType::OBJECT && args[0].obj && args[1].type == ValueType::OBJECT) {
            InterpreterObject* viewObj = (InterpreterObject*)args[0].obj;
            windroid::view::View* view = viewObj ? (windroid::view::View*)viewObj->nativeHandle : nullptr;
            InterpreterObject* listener = (InterpreterObject*)args[1].obj;
            
            int controlId = view ? view->getId() : 0;
            if (controlId != 0 && listener != nullptr) {
                clickListeners[controlId] = listener;
                if (view) {
                    view->setOnClickListener([controlId]() {
                        WindowManager::triggerClickCallback(controlId);
                    });
                }
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

    auto setOnLongClickListenerStub = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        // We're just stubbing this to avoid crash, no actual long-click handling implemented yet
        if (args.size() >= 2) {
            Logger::d("StubRegistry", "Executed: setOnLongClickListener");
        }
        return false;
    };
    stubs["Landroid/view/View;->setOnLongClickListener(Landroid/view/View$OnLongClickListener;)V"] = setOnLongClickListenerStub;
    stubs["Landroid/widget/Button;->setOnLongClickListener(Landroid/view/View$OnLongClickListener;)V"] = setOnLongClickListenerStub;
    stubs["Landroid/widget/TextView;->setOnLongClickListener(Landroid/view/View$OnLongClickListener;)V"] = setOnLongClickListenerStub;

    auto emptyStub = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) *outReturn = Value::MakeNull();
        return false;
    };

    // Added missing stubs
    stubs["Landroid/app/Activity;->onCreate(Landroid/os/Bundle;)V"] = 
        [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
            Logger::d("StubRegistry", "Executed: Activity.onCreate() -> Showing Window");
            HWND mainWnd = WindowManager::getMainWindow();
            if (mainWnd) {
                ShowWindow(mainWnd, SW_SHOW);
                UpdateWindow(mainWnd);
            }
            return false;
        };
        
    stubs["Landroid/app/Activity;->setTheme(I)V"] = emptyStub;
    stubs["Landroid/app/Fragment;-><init>()V"] = emptyStub;
    stubs["Landroid/os/Handler;->removeCallbacks(Ljava/lang/Runnable;)V"] = emptyStub;
    stubs["Landroid/view/View;->setTag(ILjava/lang/Object;)V"] = emptyStub;
    stubs["Landroid/content/Context;->getString(I)Ljava/lang/String;"] = 
        [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
            if (args.size() >= 2 && args[1].type == ValueType::INT && m_resManager) {
                int resId = args[1].i;
                std::string strVal = m_resManager->getString(resId);
                
                InterpreterObject* strObj = new InterpreterObject();
                strObj->className = "Ljava/lang/String;";
                InterpreterObject* inner = new InterpreterObject();
                inner->className = strVal;
                strObj->fields["string_value"] = Value::MakeObject(inner);
                *outReturn = Value::MakeObject(strObj);
                Logger::d("StubRegistry", "Executed: Context.getString(id=" + std::to_string(resId) + ") -> '" + strVal + "'");
            } else {
                *outReturn = Value::MakeNull();
                Logger::w("StubRegistry", "Executed: Context.getString() without valid arguments or ResourceManager");
            }
            return false;
        };
    stubs["Lz1/g;->e(Ljava/lang/Object;Ljava/lang/String;)V"] = emptyStub;
    
    stubs["Landroid/graphics/Rect;-><init>()V"] = emptyStub;
    stubs["Landroid/animation/LayoutTransition;-><init>()V"] = emptyStub;
    stubs["Landroid/animation/LayoutTransition;->disableTransitionType(I)V"] = emptyStub;
    stubs["Landroid/view/ViewGroup;->setLayoutTransition(Landroid/animation/LayoutTransition;)V"] = emptyStub;
    stubs["Landroid/widget/TextView;->setShowSoftInputOnFocus(Z)V"] = emptyStub;
    stubs["Landroid/widget/OverScroller;->abortAnimation()V"] = emptyStub;
    stubs["Landroid/widget/ImageView;->setImageResource(I)V"] = emptyStub;
    stubs["Ljava/lang/IllegalStateException;-><init>(Ljava/lang/String;)V"] = emptyStub;
    
    stubs["Landroidx/recyclerview/widget/RecyclerView;->r(Landroid/view/View;)V"] = emptyStub;
    stubs["Landroid/view/View;->clearAnimation()V"] = emptyStub;
    stubs["Landroid/widget/TextView;->addTextChangedListener(Landroid/text/TextWatcher;)V"] = emptyStub;
    stubs["Landroid/view/ViewGroup;->requestLayout()V"] = emptyStub;
    stubs["Ljava/lang/ThreadLocal;-><init>()V"] = emptyStub;
    stubs["Landroid/util/SparseArray;-><init>()V"] = emptyStub;
    stubs["Landroid/util/SparseArray;->size()I"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) *outReturn = Value::MakeInt(0);
        return false;
    };
    stubs["Landroidx/recyclerview/widget/RecyclerView;->M(Landroid/view/View;)Lm0/g0;"] = emptyStub;
    stubs["Landroid/view/View;->getLayoutParams()Landroid/view/ViewGroup$LayoutParams;"] = emptyStub;
    stubs["Lz1/g;->b(Ljava/lang/Object;)V"] = emptyStub;
    stubs["Lz1/g;->d(Ljava/lang/Object;Ljava/lang/String;)V"] = emptyStub;
    stubs["Lz1/g;->a(Ljava/lang/Object;Ljava/lang/Object;)Z"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) {
            if (args.size() >= 2 && args[0].type == ValueType::OBJECT && args[1].type == ValueType::OBJECT) {
                *outReturn = Value::MakeInt(args[0].obj == args[1].obj ? 1 : 0);
            } else {
                *outReturn = Value::MakeInt(0);
            }
        }
        return false;
    };
    stubs["Landroid/view/View;->setEnabled(Z)V"] = emptyStub;
    stubs["Landroid/app/Activity;->getResources()Landroid/content/res/Resources;"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) *outReturn = Value::MakeObject(new InterpreterObject());
        return false;
    };
    stubs["Landroid/content/res/Resources;->getConfiguration()Landroid/content/res/Configuration;"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) *outReturn = Value::MakeObject(new InterpreterObject());
        return false;
    };
    stubs["Landroid/content/res/Resources;->getDisplayMetrics()Landroid/util/DisplayMetrics;"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) *outReturn = Value::MakeObject(new InterpreterObject());
        return false;
    };
    stubs["Landroid/view/View;->requestFocus()Z"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) *outReturn = Value::MakeInt(1);
        return false;
    };
    stubs["Landroid/view/View;->getPaddingRight()I"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) *outReturn = Value::MakeInt(0);
        return false;
    };
    stubs["Landroid/view/View;->getPaddingLeft()I"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) *outReturn = Value::MakeInt(0);
        return false;
    };
    stubs["Landroid/widget/TextView;->setMinWidth(I)V"] = emptyStub;
    stubs["Landroid/view/View;->setAccessibilityDelegate(Landroid/view/View$AccessibilityDelegate;)V"] = emptyStub;

    
    stubs["Landroid/view/View;->removeCallbacks(Ljava/lang/Runnable;)Z"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) *outReturn = Value::MakeInt(0);
        return false;
    };
    
    stubs["Ljava/lang/Enum;->compareTo(Ljava/lang/Enum;)I"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) *outReturn = Value::MakeInt(1);
        return false;
    };
    stubs["Ljava/lang/Object;->toString()Ljava/lang/String;"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) {
            std::string text = "";
            if (args.size() > 0 && args[0].type == ValueType::OBJECT && args[0].obj) {
                InterpreterObject* obj = (InterpreterObject*)args[0].obj;
                auto it = obj->fields.find("string_value");
                if (it != obj->fields.end() && it->second.type == ValueType::OBJECT) {
                    text = ((InterpreterObject*)it->second.obj)->className;
                }
            }
            InterpreterObject* strObj = new InterpreterObject();
            strObj->className = "Ljava/lang/String;";
            InterpreterObject* inner = new InterpreterObject();
            inner->className = text;
            strObj->fields["string_value"] = Value::MakeObject(inner);
            *outReturn = Value::MakeObject(strObj);
        }
        return false;
    };
    
    stubs["Landroid/widget/EditText;->getText()Landroid/text/Editable;"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (args.size() >= 1 && args[0].type == ValueType::OBJECT && args[0].obj) {
            InterpreterObject* viewObj = (InterpreterObject*)args[0].obj;
            windroid::view::View* view = (windroid::view::View*)viewObj->nativeHandle;
            if (view) {
                std::string text = "stubbed_text";
                windroid::widget::TextView* tv = dynamic_cast<windroid::widget::TextView*>(view);
                if (tv) {
                    std::wstring wtext = tv->getText();
                    text = std::string(wtext.begin(), wtext.end());
                }
                
                InterpreterObject* editableObj = new InterpreterObject();
                editableObj->className = "Landroid/text/Editable;";
                InterpreterObject* inner = new InterpreterObject();
                inner->className = text;
                editableObj->fields["string_value"] = Value::MakeObject(inner);
                
                if (outReturn) *outReturn = Value::MakeObject(editableObj);
                return false;
            }
        }
        if (outReturn) *outReturn = Value::MakeNull();
        return false;
    };

    stubs["Ljava/lang/Integer;->parseInt(Ljava/lang/String;)I"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) {
            int parsed = 0;
            if (args.size() > 0 && args[0].type == ValueType::OBJECT && args[0].obj) {
                InterpreterObject* strObj = (InterpreterObject*)args[0].obj;
                auto it = strObj->fields.find("string_value");
                if (it != strObj->fields.end() && it->second.type == ValueType::OBJECT) {
                    std::string text = ((InterpreterObject*)it->second.obj)->className;
                    try {
                        parsed = std::stoi(text);
                    } catch (...) {
                        // ignore parse errors for now
                    }
                }
            }
            *outReturn = Value::MakeInt(parsed);
        }
        return false;
    };
    
    stubs["Ljava/lang/Integer;->valueOf(I)Ljava/lang/Integer;"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) {
            int val = args.size() > 0 ? args[0].i : 0;
            InterpreterObject* intObj = new InterpreterObject();
            intObj->className = "Ljava/lang/Integer;";
            intObj->fields["value"] = Value::MakeInt(val);
            *outReturn = Value::MakeObject(intObj);
        }
        return false;
    };
    
    stubs["Ljava/lang/Integer;->intValue()I"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) {
            int val = 0;
            if (args.size() > 0 && args[0].type == ValueType::OBJECT && args[0].obj) {
                InterpreterObject* intObj = (InterpreterObject*)args[0].obj;
                auto it = intObj->fields.find("value");
                if (it != intObj->fields.end() && it->second.type == ValueType::INT) {
                    val = it->second.i;
                }
            }
            *outReturn = Value::MakeInt(val);
        }
        return false;
    };

    stubs["Ljava/lang/Enum;->ordinal()I"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) *outReturn = Value::MakeInt(0); 
        return false;
    };
    
    stubs["Ljava/lang/String;->valueOf(I)Ljava/lang/String;"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) {
            int val = args.size() > 0 ? args[0].i : 0;
            InterpreterObject* strObj = new InterpreterObject();
            strObj->className = "Ljava/lang/String;";
            InterpreterObject* inner = new InterpreterObject();
            inner->className = std::to_string(val);
            strObj->fields["string_value"] = Value::MakeObject(inner);
            *outReturn = Value::MakeObject(strObj);
        }
        return false;
    };

    stubs["Ljava/lang/String;->equals(Ljava/lang/Object;)Z"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) {
            bool isEqual = false;
            if (args.size() >= 2 && args[0].type == ValueType::OBJECT && args[1].type == ValueType::OBJECT) {
                InterpreterObject* str1 = (InterpreterObject*)args[0].obj;
                InterpreterObject* str2 = (InterpreterObject*)args[1].obj;
                if (str1 && str2) {
                    std::string text1, text2;
                    auto it1 = str1->fields.find("string_value");
                    auto it2 = str2->fields.find("string_value");
                    if (it1 != str1->fields.end() && it1->second.type == ValueType::OBJECT)
                        text1 = ((InterpreterObject*)it1->second.obj)->className;
                    if (it2 != str2->fields.end() && it2->second.type == ValueType::OBJECT)
                        text2 = ((InterpreterObject*)it2->second.obj)->className;
                    
                    isEqual = (text1 == text2);
                }
            }
            *outReturn = Value::MakeInt(isEqual ? 1 : 0);
        }
        return false;
    };

    stubs["Landroid/content/Intent;->getAction()Ljava/lang/String;"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) *outReturn = Value::MakeNull(); // Null means MAIN/LAUNCHER usually
        return false;
    };

    stubs["Landroid/content/Intent;->getData()Landroid/net/Uri;"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) *outReturn = Value::MakeNull();
        return false;
    };

    stubs["Landroid/app/Activity;->getIntent()Landroid/content/Intent;"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) {
            InterpreterObject* intentObj = new InterpreterObject();
            intentObj->className = "Landroid/content/Intent;";
            *outReturn = Value::MakeObject(intentObj);
        }
        return false;
    };

    stubs["Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (args.size() >= 2 && args[0].type == ValueType::OBJECT && args[0].obj && args[1].type == ValueType::OBJECT && args[1].obj) {
            InterpreterObject* viewObj = (InterpreterObject*)args[0].obj;
            InterpreterObject* strObj = (InterpreterObject*)args[1].obj;
            windroid::view::View* view = (windroid::view::View*)viewObj->nativeHandle;
            
            std::string text = "stubbed";
            auto it = strObj->fields.find("string_value");
            if (it != strObj->fields.end() && it->second.type == ValueType::OBJECT) {
                text = ((InterpreterObject*)it->second.obj)->className;
            }
            
            if (view) {
                auto textView = dynamic_cast<windroid::widget::TextView*>(view);
                if (textView) {
                    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &text[0], (int)text.size(), NULL, 0);
                    std::wstring wstrTo(size_needed, 0);
                    MultiByteToWideChar(CP_UTF8, 0, &text[0], (int)text.size(), &wstrTo[0], size_needed);
                    
                    textView->setText(wstrTo);
                    Logger::i("StubRegistry", "Updated TextView text to: " + text);
                }
            }
        }
        return false;
    };
}
