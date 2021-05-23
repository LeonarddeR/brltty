GRADLE_WRAPPER_COMMAND = ./gradlew -q
GRADLE_DUMP_COMMAND = aapt dump --values
GRADLE_SYMLINK_COMMAND = ln -s -f

GRADLE_ROOT_NAME = brltty
GRADLE_CORE_NAME = core
GRADLE_APP_NAME = app
GRADLE_API_NAME = api

GRADLE_DEBUG_VARIANT = debug
GRADLE_RELEASE_VARIANT = release

GRADLE_ALL_ABI = universal
GRADLE_ARM32_ABI = armeabi-v7a
GRADLE_ARM64_ABI = arm64-v8a
GRADLE_X8632_ABI = x86
GRADLE_X8664_ABI = x86_64

GRADLE_BUILD_DIRECTORY = app/build
GRADLE_OUTPUT_DIRECTORY = $(GRADLE_BUILD_DIRECTORY)/outputs

GRADLE_PACKAGE_DIRECTORY = $(GRADLE_OUTPUT_DIRECTORY)/apk
GRADLE_PACKAGE_EXTENSION = apk
GRADLE_DEBUG_PACKAGE = $(GRADLE_PACKAGE_DIRECTORY)/$(GRADLE_DEBUG_VARIANT)/$(GRADLE_APP_NAME)-$(GRADLE_ALL_ABI)-$(GRADLE_DEBUG_VARIANT).$(GRADLE_PACKAGE_EXTENSION)
GRADLE_RELEASE_PACKAGE = $(GRADLE_PACKAGE_DIRECTORY)/$(GRADLE_RELEASE_VARIANT)/$(GRADLE_APP_NAME)-$(GRADLE_ALL_ABI)-$(GRADLE_RELEASE_VARIANT).$(GRADLE_PACKAGE_EXTENSION)

GRADLE_BUNDLE_DIRECTORY = $(GRADLE_OUTPUT_DIRECTORY)/bundle
GRADLE_BUNDLE_EXTENSION = aab
GRADLE_DEBUG_BUNDLE = $(GRADLE_BUNDLE_DIRECTORY)/$(GRADLE_DEBUG_VARIANT)/$(GRADLE_APP_NAME)-$(GRADLE_DEBUG_VARIANT).$(GRADLE_BUNDLE_EXTENSION)
GRADLE_RELEASE_BUNDLE = $(GRADLE_BUNDLE_DIRECTORY)/$(GRADLE_RELEASE_VARIANT)/$(GRADLE_APP_NAME)-$(GRADLE_RELEASE_VARIANT).$(GRADLE_BUNDLE_EXTENSION)

GRADLE_REPORT_DIRECTORY = $(GRADLE_BUILD_DIRECTORY)/reports
GRADLE_REPORT_NAME = lint
GRADLE_REPORT_TYPE = results
GRADLE_REPORT_EXTENSION = html
GRADLE_DEBUG_REPORT = $(GRADLE_REPORT_DIRECTORY)/$(GRADLE_REPORT_NAME)-$(GRADLE_REPORT_TYPE)-$(GRADLE_DEBUG_VARIANT).$(GRADLE_REPORT_EXTENSION)
GRADLE_RELEASE_REPORT = $(GRADLE_REPORT_DIRECTORY)/$(GRADLE_REPORT_NAME)-$(GRADLE_REPORT_TYPE)-$(GRADLE_RELEASE_VARIANT).$(GRADLE_REPORT_EXTENSION)

GRADLE_DUMP_EXTENSION = dump
GRADLE_DUMP_NAME = $(@:-dump=)
GRADLE_DUMP_FILE = $(GRADLE_ROOT_NAME)-$(GRADLE_DUMP_NAME).$(GRADLE_DUMP_EXTENSION)
