#include "slang.h"
#include "slang-com-ptr.h"
#include <iostream>
#include <print>
#include <vector>

using namespace slang;

int main() {
	Slang::ComPtr<slang::IGlobalSession> globalSession;
  SlangGlobalSessionDesc desc = {};
  createGlobalSession(&desc, globalSession.writeRef());

  std::vector<const char*> searchPaths = {
    "shaders/",
  };

  // compile utils.slang to utils.slang-module
  Slang::ComPtr<ISession> session;
  {
    std::vector<PreprocessorMacroDesc> macros = {
      {"GAMMA", "1.7"},
    };
    SessionDesc sessionDesc{
      .searchPaths = searchPaths.data(),
      .searchPathCount = static_cast<SlangInt>(searchPaths.size()),

      .preprocessorMacros = macros.data(),
      .preprocessorMacroCount = static_cast<SlangInt>(macros.size()),
    };
    globalSession->createSession(sessionDesc, session.writeRef());
  }

  std::string moduleName("utils");

  Slang::ComPtr<IBlob> diagnostics;
  Slang::ComPtr<IModule> utilsModule;
  utilsModule = session->loadModule(moduleName.c_str(), diagnostics.writeRef());
  if (diagnostics) {
    std::cout << (const char*)diagnostics->getBufferPointer() << "\n";
    std::cout << "1 ---------------------------------------------\n";
  }

  utilsModule->writeToFile("shaders/utils.slang-module");

  // compile test.slang target that uses utils.slang-module
  TargetDesc targetDesc{.format = SLANG_WGSL};
  {
    std::vector<PreprocessorMacroDesc> macros = {
      {"EMOJI"},
    };
    SessionDesc sessionDesc{
      .targets = &targetDesc,
      .targetCount = 1,

      .searchPaths = searchPaths.data(),
      .searchPathCount = static_cast<SlangInt>(searchPaths.size()),

      .preprocessorMacros = macros.data(),
      .preprocessorMacroCount = static_cast<SlangInt>(macros.size()),
    };
    globalSession->createSession(sessionDesc, session.writeRef());
  }

  moduleName = "test";

  Slang::ComPtr<IModule> testModule;
  testModule = session->loadModule(moduleName.c_str(), diagnostics.writeRef());
  if (diagnostics) {
    std::cout << (const char*)diagnostics->getBufferPointer() << "\n";
    std::cout << "2 ---------------------------------------------\n";
  }

  Slang::ComPtr<IBlob> codeBlob;
  testModule->getTargetCode(0, codeBlob.writeRef(), diagnostics.writeRef());
  if (diagnostics) {
    std::cout << (const char*)diagnostics->getBufferPointer() << "\n";
    std::cout << "3 ---------------------------------------------\n";
  }
  if (codeBlob) {
    std::cout << (const char*)codeBlob->getBufferPointer() << "\n";
  }
}
