/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "LiveValue.h"

#include "embedded/Fonts.h"

#include <visage/app.h>
#include <visage/widgets.h>

namespace {

visage::EmbeddedFile getFont() {
    return resources::fonts::DroidSansMono_ttf;
}

visage::ApplicationWindow& getWindow() {
    static std::unique_ptr<visage::ApplicationWindow> window;

    if (window != nullptr)
        return *window;

    window = std::make_unique<visage::ApplicationWindow>();

    window->setTitle("Live Values");
    window->onDraw() = [](visage::Canvas& c) {
        c.setColor(0xffffffff);
        c.fill(0, 0, window->width(), window->height());
    };

    window->onResize() = [] {
        for (uint i = 0; i < window->children().size(); ++i) {
            const auto margin = 1;
            const auto h = static_cast<float>(window->height()) / window->children().size() - 2 * margin;
            const auto w = static_cast<float>(window->width()) - 2 * margin;
            window->children()[i]->setBounds(margin, static_cast<int>(i * h + (i + 1) * margin),
                                             static_cast<int>(w), static_cast<int>(h));
        }
    };

    return *window;
}

}

LiveValue::LiveValue(const std::string& name, float initialValue) : mValue(initialValue) {
    constexpr auto kLabelHeight = 50;

    auto label = std::make_unique<visage::Frame>();
    label->onDraw() = [name, l = label.get()](visage::Canvas& c) {
        c.setColor(0xff000000);
        c.text(name, { 28, getFont() }, visage::Font::kBottom, 0, 0, l->width(), l->height());
    };

    auto textEditor = std::make_unique<visage::TextEditor>(name);
    textEditor->setFont({ 32, getFont() });
    textEditor->setJustification(visage::Font::kCenter);
    textEditor->setText(initialValue);
    textEditor->onTextChange() = [this, te = textEditor.get()] {
        const auto value = te->text().toFloat();
        mValue.store(value, std::memory_order_relaxed);
    };

    auto frame = std::make_unique<visage::Frame>();
    frame->onResize() = [f = frame.get(), l = label.get(), te = textEditor.get()] {
        l->setBounds(0, 0, f->width(), kLabelHeight);
        constexpr auto margin = 10;
        te->setBounds(0, kLabelHeight + margin, f->width(), f->height() - kLabelHeight - margin);
    };

    frame->addChild(std::move(label));
    frame->addChild(std::move(textEditor));

    getWindow().addChild(std::move(frame));
    getWindow().setWindowDimensions(160, 100 * static_cast<int>(getWindow().children().size()));
    getWindow().setOnTop(true);
    getWindow().show();
}
