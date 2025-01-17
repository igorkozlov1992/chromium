// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/webapk/webapk_update_manager.h"

#include <jni.h>

#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "chrome/browser/android/webapk/webapk_installer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "content/public/browser/browser_thread.h"
#include "jni/WebApkUpdateManager_jni.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/android/java_bitmap.h"
#include "url/gurl.h"

using base::android::JavaParamRef;

// static
bool WebApkUpdateManager::Register(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

// static
void WebApkUpdateManager::OnBuiltWebApk(const std::string& id,
                                        bool success,
                                        const std::string& webapk_package) {
  JNIEnv* env = base::android::AttachCurrentThread();

  if (success) {
    DVLOG(1)
        << "Sent request to update WebAPK to server. Seems to have worked.";
  } else {
    LOG(WARNING) << "Server request to update WebAPK failed.";
  }

  base::android::ScopedJavaLocalRef<jstring> java_id =
      base::android::ConvertUTF8ToJavaString(env, id);
  Java_WebApkUpdateManager_onBuiltWebApk(env, java_id.obj(), success);
}

// static JNI method.
static void UpdateAsync(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    const JavaParamRef<jstring>& java_id,
    const JavaParamRef<jstring>& java_start_url,
    const JavaParamRef<jstring>& java_scope,
    const JavaParamRef<jstring>& java_name,
    const JavaParamRef<jstring>& java_short_name,
    const JavaParamRef<jstring>& java_best_icon_url,
    const JavaParamRef<jstring>& java_best_icon_murmur2_hash,
    const JavaParamRef<jobject>& java_best_icon_bitmap,
    const JavaParamRef<jobjectArray>& java_icon_urls,
    jint java_display_mode,
    jint java_orientation,
    jlong java_theme_color,
    jlong java_background_color,
    const JavaParamRef<jstring>& java_web_manifest_url,
    const JavaParamRef<jstring>& java_webapk_package,
    jint java_webapk_version) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  Profile* profile = ProfileManager::GetLastUsedProfile();
  if (profile == nullptr) {
    NOTREACHED() << "Profile not found.";
    return;
  }

  std::string id(ConvertJavaStringToUTF8(env, java_id));
  GURL start_url(ConvertJavaStringToUTF8(env, java_start_url));
  GURL scope(ConvertJavaStringToUTF8(env, java_scope));
  GURL web_manifest_url(ConvertJavaStringToUTF8(env, java_web_manifest_url));
  GURL best_icon_url(ConvertJavaStringToUTF8(env, java_best_icon_url));
  ShortcutInfo info(start_url);
  info.scope = scope;
  info.name = ConvertJavaStringToUTF16(env, java_name);
  info.short_name = ConvertJavaStringToUTF16(env, java_short_name);
  info.display = static_cast<blink::WebDisplayMode>(java_display_mode);
  info.orientation =
      static_cast<blink::WebScreenOrientationLockType>(java_orientation);
  info.theme_color = (long)java_theme_color;
  info.background_color = (long)java_background_color;
  info.best_icon_url = best_icon_url;
  info.manifest_url = web_manifest_url;

  base::android::AppendJavaStringArrayToStringVector(
      env, java_icon_urls.obj(), &info.icon_urls);

  gfx::JavaBitmap java_bitmap_lock(java_best_icon_bitmap);
  SkBitmap best_icon_bitmap =
      gfx::CreateSkBitmapFromJavaBitmap(java_bitmap_lock);
  best_icon_bitmap.setImmutable();

  std::string best_icon_murmur2_hash;
  ConvertJavaStringToUTF8(env, java_best_icon_murmur2_hash,
                          &best_icon_murmur2_hash);

  std::string webapk_package;
  ConvertJavaStringToUTF8(env, java_webapk_package, &webapk_package);

  WebApkInstaller* installer = new WebApkInstaller(info, best_icon_bitmap);
  installer->UpdateAsync(
      profile,
      base::Bind(&WebApkUpdateManager::OnBuiltWebApk, id),
      best_icon_murmur2_hash, webapk_package, java_webapk_version);
}
