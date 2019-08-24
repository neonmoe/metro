// Most of this is based on the GLFW code in:
// vendor/raylib/external/glfw/cocoa_init.m

#include <sys/param.h> // For MAXPATHLEN
#include <CoreFoundation/CoreFoundation.h>
#include "raylib.h"

const char *GetResourcePathForMacOS(const char *resourcePath) {
    char resourcesPath[MAXPATHLEN];
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (!bundle) {
        printf("Warning: couldn't get main bundle, using relative paths as a fallback.\n");
        return resourcePath;
    }
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(bundle);
    if (!CFURLGetFileSystemRepresentation(resourcesURL,
                                          true,
                                          (UInt8 *)resourcesPath,
                                          MAXPATHLEN)) {
        CFRelease(resourcesURL);
        printf("Warning: couldn't get file path from bundle url, using relative paths as a fallback.\n");
        return resourcePath;
    }
    CFRelease(resourcesURL);
    return TextFormat("%s/%s", resourcesPath, resourcePath);
}
