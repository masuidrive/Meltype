// SPDX-License-Identifier: GPL-3.0-or-later
// Local Fcitx5 adapter for Meltype.
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputpanel.h>
#include <fcitx/instance.h>
#include <fcitx/candidatelist.h>
#include <fcitx/surroundingtext.h>
#include <fcitx/text.h>
#include <fcitx-utils/utf8.h>
#include <json/json.h>
#include <dlfcn.h>
#include <filesystem>
#include <stdexcept>
#include <cctype>
#include <sstream>
#include <algorithm>
#include "candidate-page.h"
using namespace fcitx;
struct Native {
    void *library;
    int (*init)(const char*, const char*);
    void* (*create)(); void (*destroy)(void*);
    char* (*key)(void*,int,int,int,const char*,const char*);
    char* (*commit)(void*); char* (*select)(void*,int);
    void (*free)(void*);
    template<typename T> T load(const char* name) {
        auto result = reinterpret_cast<T>(dlsym(library,name));
        if (!result) throw std::runtime_error(name);
        return result;
    }
    Native() {
        const auto root = std::string(getenv("HOME"))+"/.local/share/meltype";
        library = dlopen((root+"/libMeltypeNative.so").c_str(),RTLD_NOW|RTLD_LOCAL);
        if(!library) throw std::runtime_error(dlerror());
        init=load<decltype(init)>("meltype_init_mozc");create=load<decltype(create)>("meltype_create");
        destroy=load<decltype(destroy)>("meltype_destroy");key=load<decltype(key)>("meltype_handle_key");
        commit=load<decltype(commit)>("meltype_commit");select=load<decltype(select)>("meltype_select_candidate");free=load<decltype(free)>("meltype_free");
        if(!init((root+"/mozc/meltype_mozc_helper").c_str(),nullptr))
            throw std::runtime_error("Meltype Mozc helper initialization failed");
    }
    Json::Value result(char* pointer) {
        if(!pointer) return {};
        std::string text(pointer);free(pointer);
        Json::Value value;std::string errors;Json::CharReaderBuilder builder;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        if(!reader->parse(text.data(),text.data()+text.size(),&value,&errors))throw std::runtime_error(errors);
        return value;
    }
};
struct State:InputContextProperty {
    Native* native; void* session;
    State(Native* n):native(n),session(n->create()) { if(!session)throw std::runtime_error("Meltype session failed"); }
    ~State(){ native->destroy(session); }
};
class Meltype;
class Candidate:public CandidateWord {
    Meltype* engine_; int index_;
public:
    Candidate(Text text,Meltype* engine,int index):CandidateWord(std::move(text)),engine_(engine),index_(index){}
    void select(InputContext* ic)const override;
};
class Meltype:public InputMethodEngine {
    Instance* instance_; Native native_; FactoryFor<State> factory_;
public:
    Meltype(Instance* instance):instance_(instance),factory_([this](InputContext&){return new State(&native_);}) {
        instance_->inputContextManager().registerProperty("meltype",&factory_);
    }
    void apply(InputContext* ic,const Json::Value& value) {
        if(value.isNull())return;
        auto& panel=ic->inputPanel();panel.reset();
        for(const auto& edit:value["commits"]) {
            int count=edit.get("deleteBefore",0).asInt();
            if(count>0 && ic->surroundingText().isValid())ic->deleteSurroundingText(-count,count);
            auto text=edit.get("text","").asString();if(!text.empty())ic->commitString(text);
        }
        const auto& view=value["view"];
        if(view.isObject()) {
            auto string=view.get("text","").asString();
            Text preedit;
            if(view.get("converting",false).asBool() && !view["clauses"].empty()) {
                int index=0;for(const auto& clause:view["clauses"]) {
                    TextFormatFlags format=TextFormatFlag::Underline;
                    if(index++==view.get("selectedClause",-1).asInt())format|=TextFormatFlag::HighLight;
                    preedit.append(clause.asString(),format);
                }
            } else preedit.append(string,TextFormatFlag::Underline);
            preedit.setCursor(preedit.size());
            // Keep all composing text in the popup, including the first consonant.
            panel.setClientPreedit(Text());
            panel.setPreedit(Text());
            panel.setAuxUp(preedit);
            auto hint=view.get("hint","").asString();
            if(view.isMember("suggestion")&&!view["suggestion"].isNull())hint=view["suggestion"].asString()+"　"+hint;
            panel.setAuxDown(Text(hint));
            const auto& candidates=view["candidates"];
            if(view.get("converting",false).asBool()&&candidates.size()>1) {
                auto list=std::make_unique<CommonCandidateList>();list->setPageSize(9);
                list->setLayoutHint(CandidateLayoutHint::Vertical);
                int index=0;for(const auto& candidate:candidates)list->append<Candidate>(Text(candidate.asString()),this,index++);
                const int selected=std::clamp(view.get("selectedIndex",0).asInt(),0,index-1);
                // Global cursor assignment does not move CommonCandidateList's page.
                followCandidateCursor(*list,selected);
                panel.setAuxDown(Text(hint + "　候補 " + std::to_string(selected+1) + "/" + std::to_string(index)
                    + "　PageUp/PageDownでページ移動"));
                panel.setCandidateList(std::move(list));
            }
        }
        ic->updatePreedit();ic->updateUserInterface(UserInterfaceComponent::InputPanel);
    }
    void select(InputContext* ic,int index) {
        auto state=ic->propertyFor(&factory_);apply(ic,native_.result(native_.select(state->session,index)));
    }
    static int virtualKey(KeySym sym,int ch) {
        switch(sym) {
        case FcitxKey_Return:case FcitxKey_KP_Enter:return 0x0D;
        case FcitxKey_Tab:return 0x09;case FcitxKey_space:return 0x20;
        case FcitxKey_BackSpace:return 0x08;case FcitxKey_Delete:return 0x2E;case FcitxKey_Escape:return 0x1B;
        case FcitxKey_Left:return 0x25;case FcitxKey_Up:return 0x26;case FcitxKey_Right:return 0x27;case FcitxKey_Down:return 0x28;
        case FcitxKey_Home:return 0x24;case FcitxKey_End:return 0x23;case FcitxKey_Page_Up:return 0x21;case FcitxKey_Page_Down:return 0x22;
        case FcitxKey_F6:return 0x75;case FcitxKey_F7:return 0x76;case FcitxKey_F8:return 0x77;case FcitxKey_F9:return 0x78;case FcitxKey_F10:return 0x79;
        }
        if(ch>0&&ch<128&&std::isalnum(ch))return std::toupper(ch);
        switch(ch){case ',':return 0xBC;case '.':return 0xBE;case '-':return 0xBD;case '/':return 0xBF;case '[':return 0xDB;case ']':return 0xDD;}
        return ch>0?0x07:0;
    }
    void keyEvent(const InputMethodEntry&,KeyEvent& event)override {
        auto ic=event.inputContext();
        if(event.isRelease()||ic->capabilityFlags().testAny(CapabilityFlag::PasswordOrSensitive))return;
        auto key=event.key();auto states=key.states();
        if(states.test(KeyState::Ctrl)||states.test(KeyState::Alt)||states.test(KeyState::Super))return;
        if(key.sym()==FcitxKey_Page_Up || key.sym()==FcitxKey_Page_Down) {
            auto* list=dynamic_cast<CommonCandidateList*>(ic->inputPanel().candidateList().get());
            if(list && list->totalSize()>1) {
                const int old=std::max(0,list->globalCursorIndex());
                auto state=ic->propertyFor(&factory_);
                // A down key asks Meltype to expand lazy conversion candidates first.
                auto expanded=native_.result(native_.key(state->session,0x28,0,0,nullptr,nullptr));
                const int count=expanded["view"]["candidates"].size();
                if(count>0) {
                    const int delta=key.sym()==FcitxKey_Page_Down ? 9 : -9;
                    apply(ic,native_.result(native_.select(state->session,std::clamp(old+delta,0,count-1))));
                } else apply(ic,expanded);
                event.filterAndAccept();return;
            }
        }
        int ch=Key::keySymToUnicode(key.sym());int vk=virtualKey(key.sym(),ch);if(!vk)return;
        int mods=states.test(KeyState::Shift)?1:0;
        std::string before,after;
        const auto& surrounding=ic->surroundingText();
        if(surrounding.isValid()) {
            const auto& text=surrounding.text();auto length=utf8::length(text);auto cursor=std::min<size_t>(surrounding.cursor(),length);
            if(!text.empty()) {
                auto byte=utf8::ncharByteLength(text.begin(),cursor);
                auto start=utf8::ncharByteLength(text.begin(),cursor>20?cursor-20:0);
                auto end=utf8::ncharByteLength(text.begin(),std::min(length,cursor+20));
                before=text.substr(start,byte-start);after=text.substr(byte,end-byte);
            }
        }
        auto state=ic->propertyFor(&factory_);
        auto result=native_.result(native_.key(state->session,vk,ch,mods,
            before.empty()?nullptr:before.c_str(),after.empty()?nullptr:after.c_str()));
        apply(ic,result);if(result.get("consumed",false).asBool())event.filterAndAccept();
    }
    void reset(const InputMethodEntry&,InputContextEvent& event)override {
        auto ic=event.inputContext();auto state=ic->propertyFor(&factory_);
        apply(ic,native_.result(native_.commit(state->session)));
    }
};
void Candidate::select(InputContext* ic)const { engine_->select(ic,index_); }
class Factory:public AddonFactory {
    AddonInstance* create(AddonManager* manager)override{return new Meltype(manager->instance());}
};
FCITX_ADDON_FACTORY(Factory)
