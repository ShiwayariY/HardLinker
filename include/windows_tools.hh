#include <filesystem>
#include <optional>
#include <vector>

#include <windows.h>
#include <shlobj.h>

// bare minimum "smart COM ptr" because CComPtr unavailable in MinGW
template<typename T>
class COMOwner {
	bool m_locked = false;
	T* m_ptr;

public:
	void** assignable() { return (void**)&m_ptr; }
	bool lock(HRESULT r) {
		m_locked = SUCCEEDED(r);
		return m_locked;
	}
	T* operator->() const { return m_ptr; }
	~COMOwner() {
		if (m_locked) m_ptr->Release();
	}
};

std::filesystem::path get_directory_path(HWND hwnd) {
	if (hwnd == NULL) return {};

	COMOwner<IShellWindows> shell_windows;
	if (!shell_windows.lock(
		  CoCreateInstance(CLSID_ShellWindows, NULL, CLSCTX_ALL, IID_IShellWindows, shell_windows.assignable())))
		return {};

	VARIANT v;
	V_VT(&v) = VT_I4;
	TCHAR explorer_window_directory_path[MAX_PATH];
	explorer_window_directory_path[0] = TEXT('\0');

	for (V_I4(&v) = 0; true; V_I4(&v)++) {

		COMOwner<IDispatch> dispatch;
		if (!dispatch.lock(
			  shell_windows->Item(v, (IDispatch**)dispatch.assignable()))) break;

		COMOwner<IWebBrowserApp> webbrowser_app;
		if (!webbrowser_app.lock(dispatch->QueryInterface(
			  IID_IWebBrowserApp, webbrowser_app.assignable()))) continue;

		HWND webbrowser_hwnd;
		if (!SUCCEEDED(webbrowser_app->get_HWND((LONG_PTR*)&webbrowser_hwnd))) continue;
		if (webbrowser_hwnd != hwnd) continue;

		COMOwner<IServiceProvider> service_provider;
		if (!service_provider.lock(
			  webbrowser_app->QueryInterface(IID_IServiceProvider, service_provider.assignable()))) break;

		COMOwner<IShellBrowser> shell_browser;
		if (!shell_browser.lock(
			  service_provider->QueryService(SID_STopLevelBrowser, IID_IShellBrowser, shell_browser.assignable()))) break;

		COMOwner<IShellView> shell_view;
		if (!shell_view.lock(
			  shell_browser->QueryActiveShellView((IShellView**)shell_view.assignable()))) break;

		COMOwner<IFolderView> folder_view;
		if (!folder_view.lock(
			  shell_view->QueryInterface(IID_IFolderView, folder_view.assignable()))) break;

		COMOwner<IPersistFolder2> folder;
		if (!folder.lock(
			  folder_view->GetFolder(IID_IPersistFolder2, folder.assignable()))) break;

		LPITEMIDLIST folder_item_ids;
		if (SUCCEEDED(folder->GetCurFolder(&folder_item_ids))) {
			SHGetPathFromIDList(folder_item_ids, explorer_window_directory_path);
			CoTaskMemFree(folder_item_ids);
		}
		break;
	}
	return std::filesystem::u8path(explorer_window_directory_path);
}

std::optional<std::filesystem::path> get_foreground_window_path() {
	auto path = get_directory_path(GetForegroundWindow());
	if (!std::filesystem::is_directory(path)) return {};
	return path;
}

void select_filenames_in_dir(const std::filesystem::path& dir, const std::vector<std::filesystem::path>& files) {
	if (!std::filesystem::is_directory(dir) || files.empty()) return;

	std::vector<ITEMIDLIST*> selection;
	std::vector<const ITEMIDLIST*> selection_const;
	for (const auto& file : files) {
		const auto target = dir / file.filename();
		if (!std::filesystem::exists(target)) return;

		auto* IL = ILCreateFromPathW(target.c_str());
		selection.push_back(IL);
		selection_const.push_back(IL);
	}

	auto* dir_IL = ILCreateFromPathW(dir.c_str());
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	SHOpenFolderAndSelectItems(dir_IL, selection_const.size(), selection_const.data(), 0);
	CoUninitialize();

	ILFree(dir_IL);
	for (ITEMIDLIST* IL : selection)
		ILFree(IL);
}