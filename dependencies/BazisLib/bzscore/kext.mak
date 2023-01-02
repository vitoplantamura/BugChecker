#CC := llvm-gcc-4.2
CC := llvm-gcc-4.2
CXX := $(CC)
LD := $(CC)

KEXT_SDK := /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk

KEXTFLAGS := -arch x86_64 -fmessage-length=0 -pipe -nostdinc -fno-builtin -Wno-trigraphs -fno-exceptions -force_cpusubtype_ALL -msoft-float -fno-common -mkernel -finline -fno-keep-inline-functions -Wmissing-prototypes -Wreturn-type -Wunused-variable -Wshorten-64-to-32 -DDEBUG=1 -DKERNEL -DKERNEL_PRIVATE -DDRIVER_PRIVATE -DAPPLE -DNeXT -isysroot $(KEXT_SDK) -fasm-blocks -mmacosx-version-min=10.7 -ggdb -I/System/Library/Frameworks/Kernel.framework/PrivateHeaders -I$(KEXT_SDK)/System/Library/Frameworks/Kernel.framework/Headers
CXX_KEXTFLAGS := -fapple-kext -fcheck-new  -fno-rtti

CFLAGS += -x c $(KEXTFLAGS)
CXXFLAGS += -x c++ $(KEXTFLAGS) $(CXX_KEXTFLAGS)
LDFLAGS :=  -arch x86_64 -isysroot $(KEXT_SDK) -mmacosx-version-min=10.7 -lcpp_kext -Xlinker -kext -nostdlib -lkmodc++ -lkmod -lcc_kext
PREPROCESSOR_MACROS := BZS_KEXT
