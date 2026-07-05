#include <CoreFoundation/CoreFoundation.h>
#include <limits.h>
#include <libgen.h>
#include <mach-o/dyld.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern char **environ;

static bool file_exists(const char *path)
{
	struct stat st;
	return stat(path, &st) == 0;
}

static bool parent_dir_copy(char *path, size_t path_len)
{
	char tmp[PATH_MAX];
	size_t len = strnlen(path, path_len);

	if (len == 0 || len >= sizeof(tmp)) {
		return false;
	}

	memcpy(tmp, path, len + 1);
	char *parent = dirname(tmp);

	if (!parent || !*parent) {
		return false;
	}

	snprintf(path, path_len, "%s", parent);
	return true;
}

static void show_alert(const char *message)
{
	CFStringRef header = CFStringCreateWithCString(NULL, "PERFECT DARK RENAISSANCE", kCFStringEncodingUTF8);
	CFStringRef body = CFStringCreateWithCString(NULL, message, kCFStringEncodingUTF8);

	CFUserNotificationDisplayAlert(
		0,
		kCFUserNotificationStopAlertLevel,
		NULL,
		NULL,
		NULL,
		header,
		body,
		NULL,
		NULL,
		NULL,
		NULL
	);

	if (header) {
		CFRelease(header);
	}
	if (body) {
		CFRelease(body);
	}
}

int main(int argc, char **argv)
{
	char exe_path[PATH_MAX];
	uint32_t exe_size = sizeof(exe_path);

	if (_NSGetExecutablePath(exe_path, &exe_size) != 0) {
		show_alert("Could not resolve the app launcher path.");
		return 1;
	}

	char resolved_path[PATH_MAX];
	if (!realpath(exe_path, resolved_path)) {
		show_alert("Could not resolve the app launcher path.");
		return 1;
	}

	char project_root[PATH_MAX];
	snprintf(project_root, sizeof(project_root), "%s", resolved_path);

	if (!parent_dir_copy(project_root, sizeof(project_root)) ||
			!parent_dir_copy(project_root, sizeof(project_root)) ||
			!parent_dir_copy(project_root, sizeof(project_root)) ||
			!parent_dir_copy(project_root, sizeof(project_root))) {
		show_alert("Could not resolve the project root from the app bundle.");
		return 1;
	}

	char bundle_binary[PATH_MAX];
	char basedir[PATH_MAX];
	char moddir[PATH_MAX];
	char gexmoddir[PATH_MAX];
	char kakarikomoddir[PATH_MAX];
	char darknoonmoddir[PATH_MAX];
	char goldfinger64moddir[PATH_MAX];

	snprintf(bundle_binary, sizeof(bundle_binary), "%s/pd.arm64", dirname(resolved_path));
	snprintf(basedir, sizeof(basedir), "%s/data", project_root);
	snprintf(moddir, sizeof(moddir), "%s/mods/mod_allinone", project_root);
	snprintf(gexmoddir, sizeof(gexmoddir), "%s/mods/mod_gex", project_root);
	snprintf(kakarikomoddir, sizeof(kakarikomoddir), "%s/mods/mod_kakariko", project_root);
	snprintf(darknoonmoddir, sizeof(darknoonmoddir), "%s/mods/mod_dark_noon", project_root);
	snprintf(goldfinger64moddir, sizeof(goldfinger64moddir), "%s/mods/mod_goldfinger_64", project_root);

	if (!file_exists(bundle_binary)) {
		show_alert("Missing bundled pd.arm64 link. Rebuild the app wrapper before launching.");
		return 1;
	}

	char **child_argv = calloc((size_t)argc + 13, sizeof(char *));
	if (!child_argv) {
		show_alert("Out of memory while starting the launcher.");
		return 1;
	}

	int i = 0;
	child_argv[i++] = bundle_binary;
	child_argv[i++] = "--basedir";
	child_argv[i++] = basedir;
	child_argv[i++] = "--moddir";
	child_argv[i++] = moddir;
	child_argv[i++] = "--gexmoddir";
	child_argv[i++] = gexmoddir;
	child_argv[i++] = "--kakarikomoddir";
	child_argv[i++] = kakarikomoddir;
	child_argv[i++] = "--darknoonmoddir";
	child_argv[i++] = darknoonmoddir;
	child_argv[i++] = "--goldfinger64moddir";
	child_argv[i++] = goldfinger64moddir;

	for (int argi = 1; argi < argc; ++argi) {
		child_argv[i++] = argv[argi];
	}

	child_argv[i] = NULL;

	execve(bundle_binary, child_argv, environ);

	free(child_argv);
	show_alert("The game process failed to launch from the app wrapper.");
	return 1;
}
