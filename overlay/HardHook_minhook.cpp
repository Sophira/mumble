/* Copyright (C) 2005-2011, Thorvald Natvig <thorvald@natvig.com>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Mumble Developers nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "HardHook.h"
#include "ods.h"

#include <MinHook.h>

static LONG minhook_init_once = 0;

static const char *minhook_status_string(MH_STATUS status) {

#define MHS(x) \
	case x: \
		return #x;

	switch (status) {
		MHS(MH_UNKNOWN)
		MHS(MH_OK)
		MHS(MH_ERROR_ALREADY_INITIALIZED)
		MHS(MH_ERROR_NOT_INITIALIZED)
		MHS(MH_ERROR_ALREADY_CREATED)
		MHS(MH_ERROR_NOT_CREATED)
		MHS(MH_ERROR_ENABLED)
		MHS(MH_ERROR_DISABLED)
		MHS(MH_ERROR_NOT_EXECUTABLE)
		MHS(MH_ERROR_UNSUPPORTED_FUNCTION)
		MHS(MH_ERROR_MEMORY_ALLOC)
		MHS(MH_ERROR_MEMORY_PROTECT)
	}

#undef MHS

	return "(unknown)";
}

// EnsureMinHookInitialized ensures that the MinHook
// library is initialized. If MinHook is already
// initialized, calling this function is a no-op.
static void EnsureMinHookInitialized() {
	// Ensure MH_Initialize is only called once.
	if (InterlockedCompareExchange(&minhook_init_once, 1, 0) == 0) {
		MH_Initialize();
	}
}

/**
 * @brief Constructs a new hook without actually injecting.
 */
HardHook::HardHook()
	: m_func(NULL)
	, m_replacement(NULL)
	, call(NULL) {
	EnsureMinHookInitialized();
}

/**
 * @brief Constructs a new hook by injecting given replacement function into func.
 * @see HardHook::setup
 * @param func Funktion to inject replacement into.
 * @param replacement Function to inject into func.
 */
HardHook::HardHook(voidFunc func, voidFunc replacement) {
	EnsureMinHookInitialized();

	m_func = func;
	m_replacement = replacement;

	setup(func, replacement);
}

/**
 * @brief Makes sure the given replacement function is run once func is called.
 *
 * Uses MinHook to put the hook in place.
 *
 * @param func Pointer to function to redirect.
 * @param replacement Pointer to code to redirect to.
 */
void HardHook::setup(voidFunc func, voidFunc replacement) {
	MH_STATUS status = MH_CreateHook((LPVOID) func, (LPVOID)replacement, (LPVOID *)&call);
	if (status != MH_OK) {
		fods("HardHook: setup failed, MH_CreateHook returned %s", minhook_status_string(status));
	}

	status = MH_EnableHook((LPVOID)m_func);
	if (status != MH_OK) {
		fods("HardHook: setup failed, MH_EnableHook returned %s", minhook_status_string(status));
	}
}

void HardHook::setupInterface(IUnknown *unkn, LONG funcoffset, voidFunc replacement) {
	fods("HardHook: setupInterface: Replacing %p function #%ld", unkn, funcoffset);
	void **ptr = reinterpret_cast<void **>(unkn);
	ptr = reinterpret_cast<void **>(ptr[0]);
	setup(reinterpret_cast<voidFunc>(ptr[funcoffset]), replacement);
}

void HardHook::reset() {
	m_func = NULL;
	m_replacement = NULL;
	call = NULL;
}

/**
 * @brief Injects redirection code into the target function.
 *
 * @param force No-op in the MinHook-based HardHook implementation.
 */
void HardHook::inject(bool force) {
	if (!force) {
		// MinHook guarantees the presence of a trampoline, so
		// inject() and restore() are no-ops unless force is
		// set to true.
		return;
	}

	MH_STATUS status = MH_EnableHook((LPVOID)m_func);
	if (status != MH_OK) {
		fods("HardHook: inject() failed: MH_EnableHook returned %s", minhook_status_string(status));
	}
}

/**
 * @brief Restores the original code in a target function.
 *
 * @param force No-op in the MinHook-based HardHook implementation.
 */
void HardHook::restore(bool force) {
	if (!force) {
		// MinHook guarantees the presence of a trampoline, so
		// inject() and restore() are no-ops unless force is
		// set to true.
		return;
	}

	MH_STATUS status = MH_DisableHook((LPVOID)m_func);
	if (status != MH_OK) {
		fods("HardHook: restore() failed: MH_DisableHook returned %s", minhook_status_string(status));
	}
}

void HardHook::print() {
	fods("HardHook: unused 'print' method called for MinHook-based HardHook");
}

/**
 * @brief No-op in MinHook-based HardHook implementation.
 */
void HardHook::check() {
		fods("HardHook: unused 'check' method called for MinHook-based HardHook");
}
