#include <list>
#include <vector>
#include <cstring>
#include <pthread.h>
#include <thread>
#include <cstring>
#include <string>
#include <jni.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <dlfcn.h>
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.hpp"
#include "Menu/Menu.hpp"
#include "Menu/Jni.hpp"
#include "Includes/Macros.h"
#include "dobby.h"

// ========== متغيرات التحكم ==========
bool espPlayer = false;
bool espZombie = false;

// ========== عناوين RVA (من ملف dump.cs) ==========
// دالة OnHPUpdated في كلاس BaseNetworkPlayer
#define RVA_Player_OnHPUpdated 0xB1EA80 

// دالة Update في كلاس Zombie (لو ملقتهاش، سيبها 0 مؤقتاً)
#define RVA_Zombie_Update 0x000000 

// ========== دوال الهوك (Hook) ==========
void* (*old_Player_OnHPUpdated)(void* instance, float oldValue, float newValue);
void* (*old_Zombie_Update)(void* instance);

// دالة الهوك بتاعة اللاعب
void* Player_OnHPUpdated(void* instance, float oldValue, float newValue) {
    if (instance != NULL && espPlayer) {
        // قراءة الصحة (HP) من العنوان 0x6C
        float hp = *(float*)((uintptr_t)instance + 0x6C);
        
        if (hp > 0) {
            // نجيب الكاركتر عشان نوصل للـ Transform
            void* character = *(void**)((uintptr_t)instance + 0x68);
            if (character != NULL) {
                // هنا بنرسم المربع (ESP)
                // الكود ده محتاج مكتبة ImGui عشان يشتغل
                // ممكن نضيفه بعدين، بس ده الأساس
            }
        }
    }
    return old_Player_OnHPUpdated(instance, oldValue, newValue);
}

// دالة الهوك بتاعة الزومبي
void* Zombie_Update(void* instance) {
    if (instance != NULL && espZombie) {
        // قراءة الصحة (HP) من العنوان 0x54
        float hp = *(float*)((uintptr_t)instance + 0x54);
        
        if (hp > 0) {
            // كود الرسم للزومبي
        }
    }
    return old_Zombie_Update(instance);
}

// ========== دالة تفعيل الهوك ==========
void* hack_thread(void* arg) {
    // ننتظر لحد ما اللعبة تحمل المكتبة
    while (!isLibraryLoaded("libil2cpp.so")) {
        sleep(1);
    }
    
    // نحصل على عنوان المكتبة
    uintptr_t libBase = (uintptr_t)getAbsoluteAddress("libil2cpp.so", 0);
    
    // تفعيل الهوك للاعب
    void* addr_Player = (void*)(libBase + RVA_Player_OnHPUpdated);
    DobbyHook(addr_Player, (void*)Player_OnHPUpdated, (void**)&old_Player_OnHPUpdated);
    
    // تفعيل الهوك للزومبي
    if (RVA_Zombie_Update != 0x000000) {
        void* addr_Zombie = (void*)(libBase + RVA_Zombie_Update);
        DobbyHook(addr_Zombie, (void*)Zombie_Update, (void**)&old_Zombie_Update);
    }
    
    return NULL;
}

// ========== دالة القائمة (Menu) ==========
jobjectArray GetFeatureList(JNIEnv *env, jobject context) {
    jobjectArray ret;

    const char *features[] = {
        OBFUSCATE("Toggle_ESP Player"),
        OBFUSCATE("Toggle_ESP Zombie"),
    };

    int TotalFeatures = sizeof(features) / sizeof(features[0]);
    jclass stringClass = env->FindClass("java/lang/String");
    ret = env->NewObjectArray(TotalFeatures, stringClass, NULL);

    for (int i = 0; i < TotalFeatures; i++) {
        jstring str = env->NewStringUTF(features[i]);
        env->SetObjectArrayElement(ret, i, str);
    }

    return ret;
}

// ========== دالة التحكم في الأزرار ==========
void Changes(JNIEnv *env, jclass clazz, jobject obj, jint feature, jboolean boolean, jstring text) {
    switch (feature) {
        case 0: // ESP Player
            espPlayer = boolean;
            break;
        case 1: // ESP Zombie
            espZombie = boolean;
            break;
    }
}

// ========== دالة التحميل ==========
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    vm->GetEnv((void**)&env, JNI_VERSION_1_6);
    
    // تشغيل الثريد اللي بيفعل الهوك
    pthread_t ptid;
    pthread_create(&ptid, NULL, hack_thread, NULL);
    
    return JNI_VERSION_1_6;
}