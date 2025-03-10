/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <atomic>
#include <string>

class LiveValue
{
public:
    LiveValue(const std::string & name, float initialValue);

    LiveValue(const LiveValue & other) = delete;
    LiveValue & operator = (const LiveValue & other) = delete;

    operator float() const noexcept { return mValue.load(std::memory_order_relaxed); }
    float    get()   const noexcept { return mValue.load(std::memory_order_relaxed); }

private:
    std::atomic<float> mValue;
};

#ifndef NDEBUG
#define LIVE_VALUE(name, initialValue) static const LiveValue name(#name, initialValue)
#else
#define LIVE_VALUE(name, initialValue) constexpr float name = initialValue
#endif
