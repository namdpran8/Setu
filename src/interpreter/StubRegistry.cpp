#include "StubRegistry.h"
#include "../utils/Logger.h"
#include "../ui/WindowManager.h"
#include "../ui/LayoutInflater.h"
#include "../dex/ResourceManager.h"
#include "../view/ViewGroup.h"
#include "../widget/TextView.h"
#include "../widget/Button.h"
#include "../widget/EditText.h"
#include "Interpreter.h" // For recursive Interpreter execution
#include <windows.h>
#include <shellapi.h>

std::unordered_map<std::string, StubFunc> StubRegistry::stubs;
setu::ResourceManager* StubRegistry::m_resManager = nullptr;
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

static std::string getAndroidClassName(const setu::view::View* view) {
    if (dynamic_cast<const setu::widget::EditText*>(view)) return "Landroid/widget/EditText;";
    if (dynamic_cast<const setu::widget::Button*>(view)) return "Landroid/widget/Button;";
    if (dynamic_cast<const setu::widget::TextView*>(view)) return "Landroid/widget/TextView;";
    if (dynamic_cast<const setu::view::ViewGroup*>(view)) return "Landroid/view/ViewGroup;";
    return "Landroid/view/View;";
}

static bool findRequiredView(const std::vector<Value>& args, Value* outReturn) {
    // Generated ViewBinding helpers are static: args[0] is the root and args[1] is the required ID.
    if (args.size() < 2 || args[0].type != ValueType::OBJECT || !args[0].obj || args[1].type != ValueType::INT) {
        Logger::e("StubRegistry", "View binding helper received invalid arguments.");
        if (outReturn) *outReturn = Value::MakeNull();
        return true;
    }

    auto* rootObject = static_cast<InterpreterObject*>(args[0].obj);
    auto* rootView = rootObject ? static_cast<setu::view::View*>(rootObject->nativeHandle) : nullptr;
    const int targetId = args[1].i;
    auto foundView = rootView ? rootView->findViewById(targetId) : nullptr;
    if (!foundView) {
        Logger::e("StubRegistry", "Missing required view with ID: " + std::to_string(targetId));
        if (outReturn) *outReturn = Value::MakeNull();
        return true;
    }

    auto* viewObject = new InterpreterObject();
    viewObject->className = getAndroidClassName(foundView.get());
    viewObject->nativeHandle = foundView.get();
    if (outReturn) *outReturn = Value::MakeObject(viewObject);
    return false;
}

void StubRegistry::init(setu::ResourceManager* resManager, MultiDexManager* multiDexManager) {
    m_resManager = resManager;
    m_multiDexManager = multiDexManager;
    registerActivityStubs();
    registerViewStubs();
}

bool StubRegistry::isStubbed(const std::string& methodSignature) {
    if (stubs.find(methodSignature) != stubs.end()) return true;
    if (methodSignature == "Landroid/content/Intent;-><init>(Landroid/content/Context;Ljava/lang/Class;)V") return true;
    if (methodSignature == "Landroid/content/Intent;-><init>(Ljava/lang/String;Landroid/net/Uri;)V") return true;
    if (methodSignature == "Landroid/net/Uri;->parse(Ljava/lang/String;)Landroid/net/Uri;") return true;
    if (methodSignature.find("->startActivity(Landroid/content/Intent;)V") != std::string::npos) return true;
    if (methodSignature.find("->setContentView(I)V") != std::string::npos) return true;
    if (methodSignature.find("->findViewById(I)Landroid/view/View;") != std::string::npos) return true;
    if (methodSignature.find("Lkotlin/jvm/internal/Intrinsics;->checkNotNullParameter") == 0 ||
        methodSignature.find("Lkotlin/jvm/internal/Intrinsics;->checkNotNullExpressionValue") == 0) return true;
        
    size_t arrowPos = methodSignature.find("->");
    std::string cls = (arrowPos != std::string::npos) ? methodSignature.substr(0, arrowPos + 1) : methodSignature;
    
    // Common basic SDK classes we don't want to fail on yet
    if (cls == "Ljava/lang/Object;") return true;
    if (cls == "Ljava/lang/Class;") return true;
    if (cls == "Ljava/lang/Thread;") return true;
    if (cls == "Ljava/lang/StringBuilder;") return true;
    if (cls.find("Ljava/util/") == 0) return true;
    if (cls == "Landroid/content/Context;") return true;
    if (cls.find("Landroid/content/SharedPreferences") == 0) return true;
    if (methodSignature.find("Landroid/app/Activity;->getWindow") == 0) return true;
    if (cls == "Landroid/view/Window;") return true;
    if (cls.find("Landroidx/fragment/app/FragmentManager") == 0) return true;
    if (cls.find("Landroidx/fragment/app/FragmentTransaction") == 0) return true;
    if (methodSignature.find("Landroidx/fragment/app/FragmentActivity;->getSupportFragmentManager") == 0) return true;
    if (cls.find("Landroidx/lifecycle/") == 0) return true;
    if (cls == "Ljava/lang/Enum;") return true;
    if (cls == "Ljava/lang/Integer;") return true;
    if (cls == "Landroid/graphics/Rect;") return true;
    if (cls == "Landroid/animation/LayoutTransition;") return true;
    if (cls == "Ljava/lang/IllegalStateException;") return true;
    if (cls == "Landroid/widget/OverScroller;") return true;
    if (cls == "Landroid/widget/ImageView;") return true;
    if (cls == "Landroid/view/View;") return true;
    if (cls == "Landroid/view/ViewGroup;") return true;
    if (cls == "Landroid/widget/TextView;") return true;
    if (cls == "Lj0/w;") return true;
    if (cls == "Lz1/g;") return true;
    if (cls == "Ln0/a;") return true;
    if (cls == "Lu0/c;") return true;
    if (cls == "Lg/F;") return true;
    if (cls.find("Landroid/os/Build$VERSION;") == 0) return true;
    if (cls == "Lx0/b;") return true;
    if (cls.find("Landroidx/recyclerview/") == 0) return true;
    if (cls == "Landroid/util/SparseArray;") return true;
    if (cls == "Ljava/lang/ThreadLocal;") return true;
    
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
            return false;
        }
        if (methodSignature == "Landroid/content/Intent;-><init>(Ljava/lang/String;Landroid/net/Uri;)V") {
            Logger::i("StubRegistry", "Executed: Intent.<init>(String, Uri)");
            if (args.size() >= 3 && args[0].type == ValueType::OBJECT && args[2].type == ValueType::OBJECT) {
                InterpreterObject* intentObj = (InterpreterObject*)args[0].obj;
                InterpreterObject* uriObj = (InterpreterObject*)args[2].obj;
                if (intentObj && uriObj) {
                    intentObj->fields["action"] = args[1];
                    intentObj->fields["uri"] = args[2];
                }
            }
            return false;
        }
        if (methodSignature == "Landroid/net/Uri;->parse(Ljava/lang/String;)Landroid/net/Uri;") {
            Logger::i("StubRegistry", "Executed: Uri.parse(String)");
            if (args.size() >= 1 && args[0].type == ValueType::OBJECT) {
                InterpreterObject* uriObj = new InterpreterObject();
                uriObj->className = "Landroid/net/Uri;";
                uriObj->fields["uriString"] = args[0];
                if (outReturn) *outReturn = Value::MakeObject(uriObj);
            }
            return false;
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
                    } else if (intentObj->fields.find("uri") != intentObj->fields.end() && intentObj->fields["uri"].type == ValueType::OBJECT) {
                        InterpreterObject* uriObj = (InterpreterObject*)intentObj->fields["uri"].obj;
                        if (uriObj && uriObj->fields.find("uriString") != uriObj->fields.end() && uriObj->fields["uriString"].type == ValueType::OBJECT) {
                            InterpreterObject* strObj = (InterpreterObject*)uriObj->fields["uriString"].obj;
                            if (strObj) {
                                std::string url = strObj->className; // Wait, string's value is usually stored differently? 
                                // Actually, in our stub strings are sometimes just objects where className holds the string, or we have a special string class. 
                                // Let's check how string is extracted.
                                // It seems strObj->className is used for targetClass above: std::string targetClassName = strObj->className;
                                // Wait, a String object usually has its data in a special field or we use className for it in this mock engine?
                                // Assuming className holds the string based on line 152: std::string targetClassName = strObj->className;
                                std::string urlToOpen = strObj->className;
                                Logger::i("StubRegistry", "Opening URL: " + urlToOpen);
                                ShellExecuteA(0, 0, urlToOpen.c_str(), 0, 0, SW_SHOW);
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
                    auto layoutTree = m_resManager->getLayout(layoutId);
                    if (layoutTree) {
                        android::ResXMLParser parser(*layoutTree);
                        parser.restart();
                        Logger::i("StubRegistry", "Inflating layout...");
                        RECT rect;
                        GetClientRect(WindowManager::getMainWindow(), &rect);
                        int pW = rect.right > 0 ? rect.right : 400;
                        int pH = rect.bottom > 0 ? rect.bottom : 800;
                        auto rootView = setu::LayoutInflater::inflate(&parser, m_resManager);
                        WindowManager::setRootView(rootView);
                    }
                }
            }
            return false;
        }

        if (methodSignature.find("->findViewById(I)Landroid/view/View;") != std::string::npos) {
            if (args.size() >= 2 && args[1].type == ValueType::INT) {
                int id = args[1].i;
                Logger::d("StubRegistry", "Executed: findViewById(id=" + std::to_string(id) + ")");
                
                setu::view::View* childView = nullptr;
                std::function<setu::view::View*(setu::view::View*, int)> findViewRecursive = [&](setu::view::View* parent, int searchId) -> setu::view::View* {
                    if (!parent) return nullptr;
                    if (parent->getId() == searchId) return parent;
                    
                    auto viewGroup = dynamic_cast<setu::view::ViewGroup*>(parent);
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
                    viewObj->className = getAndroidClassName(childView);
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
            if (args.size() > 0 && args[0].type == ValueType::OBJECT && args[0].obj) {
                InterpreterObject* sb = (InterpreterObject*)args[0].obj;
                
                if (methodSignature.find("-><init>") != std::string::npos) {
                    InterpreterObject* inner = new InterpreterObject();
                    inner->className = ""; // Start empty
                    if (args.size() >= 2) {
                        // <init>(String)
                        if (args[1].type == ValueType::OBJECT && args[1].obj) {
                            InterpreterObject* str = (InterpreterObject*)args[1].obj;
                            if (str->fields.count("string_value")) {
                                InterpreterObject* strInner = (InterpreterObject*)str->fields["string_value"].obj;
                                if (strInner) inner->className = strInner->className;
                            }
                        }
                    }
                    sb->fields["string_value"] = Value::MakeObject(inner);
                }
                else if (methodSignature.find("->append") != std::string::npos) {
                    if (args.size() >= 2 && sb->fields.count("string_value")) {
                        InterpreterObject* inner = (InterpreterObject*)sb->fields["string_value"].obj;
                        if (inner) {
                            std::string toAppend = "";
                            if (args[1].type == ValueType::OBJECT && args[1].obj) {
                                InterpreterObject* argObj = (InterpreterObject*)args[1].obj;
                                if ((argObj->className == "Ljava/lang/String;" || argObj->className == "java.lang.String") && argObj->fields.count("string_value")) {
                                    toAppend = ((InterpreterObject*)argObj->fields["string_value"].obj)->className;
                                } else {
                                    toAppend = argObj->className; // Fallback
                                }
                            } else if (args[1].type == ValueType::INT) {
                                toAppend = std::to_string(args[1].i);
                            } else if (args[1].type == ValueType::FLOAT) {
                                toAppend = std::to_string(args[1].f);
                            }
                            inner->className += toAppend;
                        }
                    }
                    if (outReturn) *outReturn = args[0];
                }
                else if (methodSignature.find("->toString") != std::string::npos) {
                    if (outReturn) {
                        InterpreterObject* strObj = new InterpreterObject();
                        strObj->className = "Ljava/lang/String;";
                        InterpreterObject* innerStr = new InterpreterObject();
                        if (sb->fields.count("string_value")) {
                            InterpreterObject* inner = (InterpreterObject*)sb->fields["string_value"].obj;
                            if (inner) innerStr->className = inner->className;
                        }
                        strObj->fields["string_value"] = Value::MakeObject(innerStr);
                        *outReturn = Value::MakeObject(strObj);
                    }
                }
            } else {
                // Failsafe
                if (outReturn) *outReturn = Value::MakeNull();
            }
            return false;
        }
        if (methodSignature.find("Landroid/content/Context;->getPackageName") != std::string::npos) {
            if (outReturn) *outReturn = Value::MakeNull();
            return false;
        }
        // getString(int resId) is inherited from Context - any Activity/Fragment subclass may call it
        // under its own class name, so we need a wildcard fallback here.
        if (methodSignature.find("->getString(I)Ljava/lang/String;") != std::string::npos) {
            if (args.size() >= 2 && args[1].type == ValueType::INT && m_resManager) {
                int resId = args[1].i;
                std::string strVal = m_resManager->getString(resId);
                InterpreterObject* strObj = new InterpreterObject();
                strObj->className = "Ljava/lang/String;";
                InterpreterObject* inner = new InterpreterObject();
                inner->className = strVal;
                strObj->fields["string_value"] = Value::MakeObject(inner);
                if (outReturn) *outReturn = Value::MakeObject(strObj);
                Logger::d("StubRegistry", "Executed: getString(id=" + std::to_string(resId) + ") -> '" + strVal + "'");
            } else {
                if (outReturn) *outReturn = Value::MakeNull();
            }
        }
        if (methodSignature.find("Ljava/lang/Object;-><init>()V") != std::string::npos) {
            return false;
        }
        
        if (methodSignature.find("Landroid/widget/EditText;->addTextChangedListener(Landroid/text/TextWatcher;)V") != std::string::npos) {
            return false;
        }

        if (methodSignature.find("Landroid/widget/RadioButton;->setChecked(Z)V") != std::string::npos) {
            if (args.size() >= 2 && args[0].type == ValueType::OBJECT && args[0].obj && args[1].type == ValueType::INT) {
                auto* btn = static_cast<setu::widget::Button*>(static_cast<InterpreterObject*>(args[0].obj)->nativeHandle);
                if (btn) {
                    // Just log it or add a placeholder since setChecked isn't fully implemented on Button yet
                    Logger::i("StubRegistry", "RadioButton.setChecked(" + std::to_string(args[1].i) + ")");
                }
            }
            return false;
        }
        
        if (methodSignature.find("Landroid/preference/PreferenceManager;->getDefaultSharedPreferences") != std::string::npos) {
            if (outReturn) *outReturn = Value::MakeNull();
            return false;
        }

        if (methodSignature.find("Lcom/google/android/material/textfield/TextInputLayout;->setHint") != std::string::npos) {
            return false;
        }

        if (methodSignature.find("Landroid/content/Context;->getString(I)Ljava/lang/String;") != std::string::npos ||
            methodSignature.find("getString(I)Ljava/lang/String;") != std::string::npos) {
            if (args.size() > 1 && m_resManager) {
                int resId = args[1].i;
                std::string strVal = m_resManager->getString(resId);
                
                InterpreterObject* strObj = new InterpreterObject();
                strObj->className = "Ljava/lang/String;";
                InterpreterObject* inner = new InterpreterObject();
                inner->className = strVal;
                strObj->fields["string_value"] = Value::MakeObject(inner);
                if (outReturn) *outReturn = Value::MakeObject(strObj);
                Logger::d("StubRegistry", "Executed: getString(id=" + std::to_string(resId) + ") -> '" + strVal + "'");
            } else {
                if (outReturn) *outReturn = Value::MakeNull();
            }
            return false;
        }
        if (methodSignature.find("Landroid/content/res/Resources;->getDimension(I)F") != std::string::npos) {
            if (args.size() > 1 && m_resManager) {
                int resId = args[1].i;
                float dimen = m_resManager->resolveDimension(resId);
                if (outReturn) *outReturn = Value::MakeFloat(dimen);
            } else {
                if (outReturn) *outReturn = Value::MakeFloat(0.0f);
            }
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

        if (methodSignature.find("Landroidx/lifecycle/") == 0) {
            if (outReturn) *outReturn = Value::MakeNull();
            return false;
        }

        if (methodSignature.find("Landroidx/fragment/app/FragmentActivity;->getSupportFragmentManager") == 0 ||
            methodSignature.find("Landroid/app/Activity;->getFragmentManager") == 0) {
            if (outReturn) *outReturn = Value::MakeObject(new InterpreterObject());
            return false;
        }

        if (methodSignature.find("Landroid/content/res/Resources;->getResourceName") == 0) {
            if (outReturn) *outReturn = Value::MakeNull();
            return false;
        }

        if (methodSignature.find("->getStackTrace()[Ljava/lang/StackTraceElement;") != std::string::npos) {
            ArrayObject* arr = new ArrayObject();
            for (int i = 0; i < 15; ++i) {
                InterpreterObject* elem = new InterpreterObject();
                elem->className = "Ljava/lang/StackTraceElement;";
                arr->elements.push_back(Value::MakeObject(elem));
            }
            if (outReturn) *outReturn = Value::MakeArray(arr);
            return false;
        }

        if (methodSignature == "Ljava/lang/StackTraceElement;->getClassName()Ljava/lang/String;") {
            InterpreterObject* strObj = new InterpreterObject();
            strObj->className = "Ljava/lang/String;";
            if (outReturn) *outReturn = Value::MakeObject(strObj);
            return false;
        }

        if (methodSignature == "Ljava/lang/StackTraceElement;->getMethodName()Ljava/lang/String;" || 
            methodSignature == "Ljava/lang/StackTraceElement;->getFileName()Ljava/lang/String;") {
            InterpreterObject* strObj = new InterpreterObject();
            strObj->className = "Ljava/lang/String;";
            if (outReturn) *outReturn = Value::MakeObject(strObj);
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
    static std::vector<std::shared_ptr<setu::view::View>> s_dynamicViews;
    
    // Basic dynamic view creation helper
    auto dynamicViewInit = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn, const std::string& className) -> bool {
        if (args.size() > 0 && args[0].type == ValueType::OBJECT && args[0].obj) {
            InterpreterObject* viewObj = (InterpreterObject*)args[0].obj;
            auto view = std::make_shared<setu::view::ViewGroup>(); // generic for now
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
            setu::view::View* view = (setu::view::View*)viewObj->nativeHandle;
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
                setu::widget::TextView* view = (setu::widget::TextView*)viewObj->nativeHandle;
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
                setu::widget::TextView* view = (setu::widget::TextView*)viewObj->nativeHandle;
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
                    auto layoutTree = m_resManager->getLayout(layoutId);
                    if (layoutTree) {
                        android::ResXMLParser parser(*layoutTree);
                        parser.restart();
                        Logger::i("StubRegistry", "Inflating layout...");
                        RECT rect;
                        GetClientRect(WindowManager::getMainWindow(), &rect);
                        int pW = rect.right > 0 ? rect.right : 400;
                        int pH = rect.bottom > 0 ? rect.bottom : 800;
                        auto rootView = setu::LayoutInflater::inflate(&parser, m_resManager);
                        WindowManager::setRootView(rootView);
                        
                        InterpreterObject* viewObj = new InterpreterObject();
                        viewObj->className = "Landroid/view/View;";
                        viewObj->nativeHandle = rootView.get();
                        if (outReturn) *outReturn = Value::MakeObject(viewObj);
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
            setu::view::View* view = viewObj ? (setu::view::View*)viewObj->nativeHandle : nullptr;
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
            Logger::e("StubRegistry", "setOnClickListener called with invalid arguments.");        }
        return false;
    };
    
    // ViewBinding generated findChildViewById stub. Keep these exact signatures; do not
    // use broad package matching because obfuscation is app-specific.
    stubs["Lq1/b;->g(Landroid/view/View;I)Landroid/view/View;"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        return findRequiredView(args, outReturn);
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
            setu::view::View* view = (setu::view::View*)viewObj->nativeHandle;
            if (view) {
                std::string text = "stubbed_text";
                setu::widget::TextView* tv = dynamic_cast<setu::widget::TextView*>(view);
                if (tv) {
                    std::wstring wtext = tv->getText();
                    text = utf16_to_utf8(wtext);
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
            setu::view::View* view = (setu::view::View*)viewObj->nativeHandle;
            
            std::string text = "stubbed";
            auto it = strObj->fields.find("string_value");
            if (it != strObj->fields.end() && it->second.type == ValueType::OBJECT) {
                text = ((InterpreterObject*)it->second.obj)->className;
            }
            
            if (view) {
                auto textView = dynamic_cast<setu::widget::TextView*>(view);
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

    stubs["Ljava/lang/String;->concat(Ljava/lang/String;)Ljava/lang/String;"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) {
            std::string text1 = "", text2 = "";
            if (args.size() > 0 && args[0].type == ValueType::OBJECT && args[0].obj) {
                InterpreterObject* str1 = (InterpreterObject*)args[0].obj;
                auto it1 = str1->fields.find("string_value");
                if (it1 != str1->fields.end() && it1->second.type == ValueType::OBJECT) text1 = ((InterpreterObject*)it1->second.obj)->className;
            }
            if (args.size() > 1 && args[1].type == ValueType::OBJECT && args[1].obj) {
                InterpreterObject* str2 = (InterpreterObject*)args[1].obj;
                auto it2 = str2->fields.find("string_value");
                if (it2 != str2->fields.end() && it2->second.type == ValueType::OBJECT) text2 = ((InterpreterObject*)it2->second.obj)->className;
            }
            InterpreterObject* strObj = new InterpreterObject();
            strObj->className = "Ljava/lang/String;";
            InterpreterObject* inner = new InterpreterObject();
            inner->className = text1 + text2;
            strObj->fields["string_value"] = Value::MakeObject(inner);
            *outReturn = Value::MakeObject(strObj);
        }
        return false;
    };

    stubs["Ljava/lang/NullPointerException;-><init>(Ljava/lang/String;)V"] = emptyStub;
    
    stubs["Landroid/view/View;->getResources()Landroid/content/res/Resources;"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) {
            InterpreterObject* resObj = new InterpreterObject();
            resObj->className = "Landroid/content/res/Resources;";
            *outReturn = Value::MakeObject(resObj);
        }
        return false;
    };

    stubs["Landroid/content/res/Resources;->getResourceName(I)Ljava/lang/String;"] = [](InterpreterState* state, const std::vector<Value>& args, Value* outReturn) -> bool {
        if (outReturn) {
            std::string resName = "unknown_resource";
            if (args.size() > 1 && args[1].type == ValueType::INT && m_resManager) {
                resName = m_resManager->getString(args[1].i);
            }
            InterpreterObject* strObj = new InterpreterObject();
            strObj->className = "Ljava/lang/String;";
            InterpreterObject* inner = new InterpreterObject();
            inner->className = resName;
            strObj->fields["string_value"] = Value::MakeObject(inner);
            *outReturn = Value::MakeObject(strObj);
        }
        return false;
    };
}




