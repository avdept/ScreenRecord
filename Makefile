.PHONY: build run-linux run-mac release clean test configure reset-tcc setup-signing

BUILD_DIR := build/default
RELEASE_DIR := build/release

configure:
	cmake --preset default

build: | $(BUILD_DIR)
	cmake --build $(BUILD_DIR)

run-linux: build
	$(BUILD_DIR)/screencopy-app

run-mac: build
	open $(BUILD_DIR)/screencopy-app.app

release:
	cmake --preset release
	cmake --build $(RELEASE_DIR)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	rm -rf build

# macOS: reset TCC entries for our bundle ID (forces a fresh permission prompt)
reset-tcc:
	tccutil reset ScreenCapture com.screencopy.app
	tccutil reset Microphone com.screencopy.app
	tccutil reset Camera com.screencopy.app

# macOS: create a self-signed "ScreenCopyDev" code-signing identity and trust it.
# Gives the app a stable signature across rebuilds so TCC permissions persist.
# Safe to re-run — exits early if the identity already exists.
setup-signing:
	@if security find-identity -v -p codesigning | grep -q ScreenCopyDev; then \
		echo "ScreenCopyDev identity already installed."; \
	else \
		echo "Creating self-signed ScreenCopyDev identity..."; \
		TMPDIR=$$(mktemp -d); \
		openssl req -x509 -newkey rsa:2048 -sha256 -days 3650 -nodes \
			-keyout $$TMPDIR/key -out $$TMPDIR/crt \
			-subj "/CN=ScreenCopyDev" \
			-addext "keyUsage=critical,digitalSignature" \
			-addext "extendedKeyUsage=critical,codeSigning" 2>/dev/null; \
		openssl pkcs12 -export -legacy \
			-inkey $$TMPDIR/key -in $$TMPDIR/crt \
			-out $$TMPDIR/p12 -passout pass:screencopy \
			-name "ScreenCopyDev"; \
		security import $$TMPDIR/p12 \
			-k ~/Library/Keychains/login.keychain-db \
			-P screencopy \
			-T /usr/bin/codesign \
			-T /usr/bin/security; \
		security add-trusted-cert -r trustRoot -p codeSign \
			-k ~/Library/Keychains/login.keychain-db $$TMPDIR/crt; \
		rm -rf $$TMPDIR; \
		echo "Done. Run 'cmake --preset default' to pick up the new identity."; \
	fi

$(BUILD_DIR):
	cmake --preset default
