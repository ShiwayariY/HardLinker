#include <iostream>
#include <filesystem>
#include <vector>
#include <optional>
#include <set>
#include <algorithm>
#include <ranges>
#include <concepts>

#include <QtCore/QMimeData>
#include <QtCore/QUrl>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtCore/QCommandLineParser>
#include <QtCore/QStringList>

#include <windows_tools.hh>

std::vector<std::filesystem::path> get_existing_files_from_clipboard() {
	auto* clip_data = QGuiApplication::clipboard()->mimeData();
	if (!clip_data->hasUrls()) return {};

	auto urls = clip_data->urls();
	std::vector<std::filesystem::path> existing_files;
	for (const auto& url : urls) {
		if (!url.isLocalFile()) continue;
		auto path = std::filesystem::u8path(url.toLocalFile().toStdString());
		if (!std::filesystem::is_regular_file(path)) continue;
		existing_files.push_back(path);
	}

	return existing_files;
}

void warn(const std::string& text) {
	QMessageBox::warning(nullptr, QApplication::applicationName(), QString::fromStdString(text));
}

bool file_not_in_dir_or_warn(const std::filesystem::path& file, const std::filesystem::path& dir) {
	if (std::filesystem::exists(dir / file.filename())) {
		warn("'" + file.filename().string() + "' already exists.\nNo hardlinks will be created.");
		return false;
	}
	return true;
}

bool same_root_or_warn(const std::filesystem::path& file, const std::filesystem::path& dir) {
	if (file.root_name() != dir.root_name()) {
		warn("'" + file.filename().string() + "' is on a different volume.\nNo hardlinks will be created.");
		return false;
	}
	return true;
}

template<typename R, typename V>
concept range_of = std::ranges::range<R>&& std::convertible_to<std::ranges::range_value_t<R>, V>;

bool hardlink_allowed(
  const range_of<std::filesystem::path> auto& source_files,
  const std::filesystem::path& dest_dir) {

	return std::ranges::all_of(source_files, [&dest_dir](const auto& file) {
		return file_not_in_dir_or_warn(file, dest_dir) && same_root_or_warn(file, dest_dir);
	});
}

struct Options {
	bool quit = false;
	bool err = false;
	bool clip_to_foreground = false;
};

bool handle_parse(QCommandLineParser& parser) {
	if (!parser.parse(QApplication::arguments())) {
		std::cerr << parser.errorText().toStdString() << "\n\n"
				  << parser.helpText().toStdString();
		return false;
	}
	return true;
}

Options handle_options() {
	QCommandLineParser parser;
	const auto ver_opt = parser.addVersionOption();
	const auto help_opt = parser.addHelpOption();
	parser.setApplicationDescription("Create hardlinks. Does not overwrite existing files.");
	parser.addOptions({ { "auto", "Create hardlinks to files in the clipboard in the foreground window directory" } });

	Options opts;
	opts.err = !handle_parse(parser);
	if (opts.err) return opts;

	if (parser.isSet(help_opt)) {
		parser.showHelp();
		opts.quit = true;
		return opts;
	}
	if (parser.isSet(ver_opt)) {
		std::cout << QApplication::applicationName().toStdString() << " "
				  << QApplication::applicationVersion().toStdString() << "\n";
		opts.quit = true;
		return opts;
	}

	if (parser.isSet("auto"))
		opts.clip_to_foreground = true;

	return opts;
}

void hardlink_from_clip_to_foreground_window() {
	const auto files = get_existing_files_from_clipboard();
	const auto dest = get_foreground_window_path();
	if (files.empty() || !dest || !hardlink_allowed(files, *dest)) return;

	for (const auto& file : files)
		std::filesystem::create_hard_link(file, *dest / file.filename());

	select_filenames_in_dir(*dest, files);
}

int main(int argc, char** argv) {
	QApplication app{ argc, argv };
	QApplication::setApplicationName("HardLinker");
	QApplication::setApplicationVersion("0.1");
	auto opts = handle_options();
	if (opts.err) return 1;
	if (opts.quit) return 0;
	if (opts.clip_to_foreground) hardlink_from_clip_to_foreground_window();
}
