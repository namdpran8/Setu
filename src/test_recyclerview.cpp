#include ^<iostream^>
#include "widget/RecyclerView.h"
#include "view/View.h"
#include "view/ViewGroup.h"
#include "view/MotionEvent.h"

using namespace setu;

class TestAdapter : public widget::Adapter {
public:
    int createCount = 0;
    int bindCount = 0;

    std::shared_ptr^<widget::ViewHolder^> onCreateViewHolder(view::ViewGroup* parent, int viewType) override {
        createCount++;
        auto v = std::make_shared^<view::View^>();
        v-^>setMeasuredDimension(100, 150 * view::View::getDisplayDensity());
        return std::make_shared^<widget::ViewHolder^>(v);
    }

    void onBindViewHolder(std::shared_ptr^<widget::ViewHolder^> holder, int position) override {
        bindCount++;
    }

    int getItemCount() override {
        return 1000;
    }
};

int main() {
    view::View::setDisplayMetrics(1.0f, 1.0f);
    auto recycler = std::make_shared^<widget::RecyclerView^>();
    auto adapter = std::make_shared^<TestAdapter^>();
    recycler-^>setAdapter(adapter);

    recycler-^>setMeasuredDimension(1000, 1000);
    recycler-^>layout(0, 0, 1000, 1000);

    std::cout ^<^< "Create count after initial layout: " ^<^< adapter-^>createCount ^<^< "\n";
    std::cout ^<^< "Bind count after initial layout: " ^<^< adapter-^>bindCount ^<^< "\n";

    recycler-^>scrollTo(0, 5000);
    recycler-^>layout(0, 0, 1000, 1000);

    std::cout ^<^< "Create count after scroll: " ^<^< adapter-^>createCount ^<^< "\n";
    std::cout ^<^< "Bind count after scroll: " ^<^< adapter-^>bindCount ^<^< "\n";

    return 0;
}
