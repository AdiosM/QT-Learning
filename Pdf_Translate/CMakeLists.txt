cmake_minimum_required(VERSION 3.21)
project(PdfTranslator VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 组件：Widgets(界面) Pdf(渲染) Network(Azure API) Sql(缓存) Test(单元测试)
find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Pdf Network Sql Test)
qt_standard_project_setup()

qt_add_executable(PdfTranslator WIN32
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/PdfViewWidget.cpp
    src/PdfViewWidget.h
    src/TranslationView.cpp
    src/TranslationView.h
    src/TranslationManager.cpp
    src/TranslationManager.h
    src/core/Translator.h
    src/core/TranslatorFactory.cpp
    src/core/MockTranslator.cpp
    src/core/MockTranslator.h
    src/core/AzureTranslator.cpp
    src/core/AzureTranslator.h
    src/core/AzureJson.cpp
    src/core/AzureJson.h
    src/core/DeepSeekTranslator.cpp
    src/core/DeepSeekTranslator.h
    src/core/DeepSeekJson.cpp
    src/core/DeepSeekJson.h
    src/core/TranslationQueue.cpp
    src/core/TranslationQueue.h
    src/core/ParagraphSplitter.cpp
    src/core/ParagraphSplitter.h
    src/core/TranslationCache.cpp
    src/core/TranslationCache.h
    src/settings/AppSettings.cpp
    src/settings/AppSettings.h
    src/settings/SettingsDialog.cpp
    src/settings/SettingsDialog.h
    src/resources/app.rc
    src/resources/resources.qrc
)
# 统一以 src/ 为根做相对包含（如 "settings/AppSettings.h"）
target_include_directories(PdfTranslator PRIVATE src)
target_link_libraries(PdfTranslator PRIVATE Qt6::Widgets Qt6::Pdf Qt6::Network Qt6::Sql)

if(MSVC)
    # MSVC 默认按系统代码页(中文 Windows 为 GBK)读源码，
    # 不加 /utf-8 会导致中文字符串字面量乱码
    target_compile_options(PdfTranslator PRIVATE /utf-8)
endif()

include(CTest)
if(BUILD_TESTING)
    # 控制台单元测试（不依赖 GUI 的核心逻辑）
    qt_add_executable(PdfTranslatorTests
        tests/testmain.cpp
        tests/TestParagraphSplitter.cpp
        tests/TestParagraphSplitter.h
        tests/TestAzureJson.cpp
        tests/TestAzureJson.h
        tests/TestDeepSeekJson.cpp
        tests/TestDeepSeekJson.h
        tests/TestTranslationCache.cpp
        tests/TestTranslationCache.h
        src/core/ParagraphSplitter.cpp
        src/core/ParagraphSplitter.h
        src/core/AzureJson.cpp
        src/core/AzureJson.h
        src/core/DeepSeekJson.cpp
        src/core/DeepSeekJson.h
        src/core/TranslationCache.cpp
        src/core/TranslationCache.h
    )
    target_include_directories(PdfTranslatorTests PRIVATE src/core)
    target_link_libraries(PdfTranslatorTests PRIVATE Qt6::Test Qt6::Sql)
    if(MSVC)
        target_compile_options(PdfTranslatorTests PRIVATE /utf-8)
    endif()
    # -o -,txt 让 QTest 结果直接输出到控制台（ctest --output-on-failure 可见）
    add_test(NAME core_tests COMMAND PdfTranslatorTests -o -,txt)
endif()
