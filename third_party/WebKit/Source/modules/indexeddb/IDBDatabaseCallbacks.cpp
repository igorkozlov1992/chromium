/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "modules/indexeddb/IDBDatabaseCallbacks.h"

#include "modules/indexeddb/IDBDatabase.h"
#include "modules/indexeddb/WebIDBDatabaseCallbacksImpl.h"

namespace blink {

IDBDatabaseCallbacks* IDBDatabaseCallbacks::create() {
  return new IDBDatabaseCallbacks();
}

IDBDatabaseCallbacks::IDBDatabaseCallbacks() : m_database(nullptr) {}

IDBDatabaseCallbacks::~IDBDatabaseCallbacks() {}

DEFINE_TRACE(IDBDatabaseCallbacks) {
  visitor->trace(m_database);
}

void IDBDatabaseCallbacks::onForcedClose() {
  if (m_database)
    m_database->forceClose();
}

void IDBDatabaseCallbacks::onVersionChange(int64_t oldVersion,
                                           int64_t newVersion) {
  if (m_database)
    m_database->onVersionChange(oldVersion, newVersion);
}

void IDBDatabaseCallbacks::onAbort(int64_t transactionId, DOMException* error) {
  if (m_database)
    m_database->onAbort(transactionId, error);
}

void IDBDatabaseCallbacks::onComplete(int64_t transactionId) {
  if (m_database)
    m_database->onComplete(transactionId);
}

void IDBDatabaseCallbacks::onChanges(
    const std::unordered_map<int32_t, std::vector<int32_t>>&
        observation_index_map,
    const WebVector<WebIDBObservation>& observations) {
  if (m_database)
    m_database->onChanges(observation_index_map, observations);
}

void IDBDatabaseCallbacks::connect(IDBDatabase* database) {
  DCHECK(!m_database);
  DCHECK(database);
  m_database = database;
}

std::unique_ptr<WebIDBDatabaseCallbacks>
IDBDatabaseCallbacks::createWebCallbacks() {
  DCHECK(!m_webCallbacks);
  std::unique_ptr<WebIDBDatabaseCallbacks> callbacks =
      WebIDBDatabaseCallbacksImpl::create(this);
  m_webCallbacks = callbacks.get();
  return callbacks;
}

void IDBDatabaseCallbacks::detachWebCallbacks() {
  if (m_webCallbacks) {
    m_webCallbacks->detach();
    m_webCallbacks = nullptr;
  }
}

void IDBDatabaseCallbacks::webCallbacksDestroyed() {
  DCHECK(m_webCallbacks);
  m_webCallbacks = nullptr;
}

}  // namespace blink
