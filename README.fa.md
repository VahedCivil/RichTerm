# RichTerm 0.3.0 — کتابخانهٔ قالب‌بندی و نمایش حرفه‌ای خروجی کنسول در C++

**RichTerm** یک کتابخانهٔ مستقل و ماژولار C++20 برای چاپ رنگی، قالب‌بندی رشته و اعداد، نشانه‌گذاری شبیه BBCode، جدول، نوار پیشرفت انیمیشنی و Status Spinner است.

این پروژه با الهام از تجربهٔ کاربری [Rich در Python](https://github.com/Textualize/rich) و الگوهای رنگ‌بندی و قالب‌بندی [ColormaCpp](https://github.com/VahedCivil/ColormaCpp) به‌صورت مستقل پیاده‌سازی شده است؛ پورت رسمی Rich نیست و به کد منبع آن وابستگی ندارد.

## قابلیت‌ها

- `Console`: توابع `print`، `println`، `write`، تشخیص ترمینال، رنگ ANSI، BBCode-style markup و Rule.
- قالب‌بندی رشته و عدد با `{}`، عرض، تراز، fill، علامت، صفرگذاری، جداکنندهٔ هزارگان، دقت اعشاری، مبناهای عددی، نماد علمی و درصد.
- رنگ‌های استاندارد و روشن، رنگ پس‌زمینه و استایل‌های Bold، Dim، Italic، Underline، Reverse و Strike با API سازگار از نظر مفهومی با ColormaCpp.
- جدول Unicode با محاسبهٔ عرض نمایشی، Wrap سلول‌ها، تراز ستون‌ها، عنوان، کپشن و رنگ مستقل خطوط.
- `Panel` با عنوان، کادر، عرض، Padding، تراز و رنگ‌بندی.
- قالب‌بندی تاریخ، زمان و مدت با `std::chrono`.
- `Console::pause` برای مکث و `Console::clear` برای پاک‌کردن ترمینال تعاملی.
- امکان تغییر رنگ نوار و Pulse در `Progress`.
- `Progress`: چند Task، نوار پیشرفت درصدی، انیمیشن pulse برای Task بدون Total، زمان سپری‌شده و به‌روزرسانی زنده در ترمینال.
- `Status`: Spinner انیمیشنی در ترمینال تعاملی.
- در خروجی Redirect شده، فایل، Pipe و CI انیمیشن غیرفعال می‌شود. Progress در شروع و پایان Snapshot متنی می‌نویسد و با `refresh()` نیز می‌توان Snapshotهای میانی ثبت کرد؛ Status نیز پیام‌های معمولی در لاگ می‌نویسد.
- CMake، هدرهای مستقل، تست CTest، مثال کامل، مستندات و مجوز MIT.

## رفع مشکل نمایش کاراکترها در ویندوز

در نسخهٔ 0.2.0، Console در صورت اتصال به کنسول واقعی ویندوز، صفحه‌کد خروجی را روی UTF-8 قرار می‌دهد و Virtual Terminal Processing را فعال می‌کند. هدف CMake نیز گزینهٔ `/utf-8` را برای MSVC منتقل می‌کند. این اصلاح برای نمایش درست کادرهای جدول و نمادهای انیمیشن ضروری است.

## ساخت و تست

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

برای Visual Studio 2026 می‌توان از Generator زیر استفاده کرد (به CMake مناسب و پشتیبان VS 2026 نیاز دارد):

```powershell
cmake -S . -B build-vs2026 -G "Visual Studio 18 2026" -A x64
cmake --build build-vs2026 --config Release --parallel
ctest --test-dir build-vs2026 -C Release --output-on-failure
```

اجرای مثال: `build\examples\Release\richterm_showcase.exe`

## نمونهٔ سریع

```cpp
#include <richterm/richterm.hpp>

int main() {
    richterm::Console console;
    console.print("Hello [bold cyan]RichTerm[/bold cyan]!");
    console.print("عدد={:,}، پی={:.12f}، هگز={:#08x}",
                   123456789, 3.141592653589793, 48879);
    console.print("{}SUCCESS{}: {} tests", richterm::Color::green,
                   richterm::Style::reset, 12);
}
```

## Progress و Status انیمیشنی

```cpp
richterm::Progress progress(console, 28, true, 12.0);
auto task = progress.add_task("Compile", 100.0);
progress.start();
for (int i = 0; i <= 100; i += 5) {
    progress.update(task, static_cast<double>(i));
    // کار اصلی برنامه
}
progress.stop();

richterm::Status status(console, "Preparing report", 10.0);
status.start();
status.update("Writing files");
status.stop("Finished");
```

انیمیشن فقط وقتی اجرا می‌شود که خروجی به ترمینال تعاملی متصل باشد. در صورت Redirect شدن خروجی، کنترل مکان‌نما و نخ انیمیشن فعال نمی‌شود و پیام‌های متنی معمولی ثبت می‌شوند. پنجرهٔ Output در بعضی IDEها TTY واقعی نیست؛ برای دیدن انیمیشن از Terminal داخلی VS Code یا Windows Terminal استفاده کنید.

نرخ Refresh باید در بازهٔ `(0, 1000]` باشد. شیء `Console` و Stream مربوط به آن باید تا پایان عمر `Progress` و `Status` باقی بمانند.

## نصب با CMake در پروژهٔ دیگر

```cmake
find_package(RichTerm CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE RichTerm::RichTerm)
```

## دامنه و محدودیت‌ها

این پروژه پورت کامل API کتابخانهٔ Python Rich نیست. Markdown کامل، Syntax Highlighting، Tracebackهای پیشرفته، Jupyter، SVG/HTML، سیستم کامل Layout و نمایشگرهای تمام‌صفحه Rich در این نسخه وجود ندارند. محاسبهٔ عرض Unicode با بازه‌های داخلی و فشرده انجام می‌شود و جایگزین کامل پایگاه دادهٔ Unicode یا رفتار اختصاصی تمام ترمینال‌ها نیست. برای محدودکردن عرض و فعال‌کردن Wrap جدول از `Table::set_max_width()` استفاده کنید.

## Panel و تاریخ/زمان

```cpp
#include <chrono>

richterm::Panel panel("متن داخل پنل در عرض تعیین‌شده می‌شکند.", "گزارش ساخت");
panel.set_width(42).set_padding(1).set_border_style("bright_blue");
console.print(panel);

const auto now = std::chrono::system_clock::now();
console.print("تاریخ: {}", richterm::format_date(now));
console.print("زمان: {}", richterm::format_time(now));
console.print("مدت: {}", richterm::format_duration(std::chrono::seconds{3661}));
```

## مکث، پاک‌کردن ترمینال و رنگ Progress

```cpp
console.pause(0.5); // seconds
console.clear();    // فقط برای ترمینال تعاملی

richterm::Progress progress(console);
progress.set_bar_color(richterm::Color::bright_green);
progress.set_pulse_color(richterm::Color::bright_cyan);
```

برای BBCode می‌توان از `[/]` برای بستن آخرین تگ باز استفاده کرد. تابع `Console::escape_markup()` نیز براکت‌های متن ورودی را Escape می‌کند.

مثال‌های مستقل در `examples/` قرار دارند. تصاویر ثابت را در `docs/images/` و GIFهای نمایشی را در `docs/gifs/` قرار دهید.

## مشارکت

از توسعه‌دهندگان برای گزارش باگ، افزودن تست، بهبود مستندات و توسعهٔ ماژول‌ها دعوت می‌کنیم. برای تغییرات بزرگ ابتدا Issue باز کنید و برای هر رفع باگ، تست Regression اضافه کنید. راهنمای [CONTRIBUTING](CONTRIBUTING.md) را بخوانید.

## کلیدواژه‌ها

`C++ terminal library`، `C++ console styling`، `Rich for C++`، `BBCode markup`، `ANSI colors`، `CMake C++20`، `Unicode console table`، `animated progress bar`، `terminal spinner`، `numeric formatting`، `ColormaCpp`، `Python Rich alternative`
