#pragma once

#include <Arduino.h>
#include <cJSON.h>

namespace SklandApi {
bool loadCredentials();
bool hasSavedAccount();
void saveCredentials(const String &cred, const String &accountToken);
cJSON *get(const char *path, const String &query = "", const String &role = "", bool transformed = false);
// Same as get(), but asks the server for a base64 "x-transformed" payload and
// replaces data with the decoded JSON object, so callers see plain fields.
cJSON *getTransformed(const char *path, const String &query = "", const String &role = "");
String uid();
String playerName();
String lastError();
uint32_t serverTimestamp();
bool downloadPortrait(const String &url, uint8_t **buffer, size_t *size);
}
