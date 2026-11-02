#include <gtest/gtest.h>
#include "Utils/TouchControls.h"

namespace Daedalus
{
	TEST(TouchControlsTest, EveryButtonHasIndependentOwnershipAndOneRelease)
	{
		TouchControls controls;
		controls.Resize(1080.f, 2200.f);
		for (size_t i = 0; i < TouchControls::ButtonCount; ++i)
		{
			const auto button = static_cast<TouchControls::Button>(i);
			const auto [x, y] = controls.ButtonCenter(button);
			controls.Down(static_cast<int>(i), x, y);
			EXPECT_TRUE(controls.Held(button));
		}
		for (size_t i = 0; i < TouchControls::ButtonCount; ++i)
		{
			const auto button = static_cast<TouchControls::Button>(i);
			controls.Release(static_cast<int>(i));
			EXPECT_FALSE(controls.Held(button));
			EXPECT_TRUE(controls.ConsumeRelease(button));
			EXPECT_FALSE(controls.ConsumeRelease(button));
		}
		EXPECT_EQ(controls.Movement(), (std::pair<float, float>{}));
		EXPECT_EQ(controls.ConsumeLook(), (std::pair<float, float>{}));
	}

	TEST(TouchControlsTest, ResetCancelsButtonsWithoutManufacturingToggleReleases)
	{
		TouchControls controls;
		controls.Resize(1000.f, 600.f);
		const auto [x, y] = controls.ButtonCenter(TouchControls::Button::Console);
		controls.Down(3, x, y);
		controls.Reset();
		controls.Release(3);
		EXPECT_FALSE(controls.Held(TouchControls::Button::Console));
		EXPECT_FALSE(controls.ConsumeRelease(TouchControls::Button::Console));
		controls.Down(4, x, y);
		controls.Release(4);
		controls.Reset();
		EXPECT_FALSE(controls.ConsumeRelease(TouchControls::Button::Console));
	}

	TEST(TouchControlsTest, IndependentPointersMoveLookAndUp)
	{
		TouchControls controls;
		controls.Resize(1000.f, 600.f);
		const auto [cx, cy] = controls.MovementCenter();
		const auto [ux, uy] = controls.UpCenter();
		controls.Down(7, cx, cy);
		controls.Down(42, 600.f, 100.f);
		controls.Down(3, ux, uy);
		controls.Move(7, cx, cy - controls.Radius());
		controls.Move(42, 620.f, 130.f);
		EXPECT_FLOAT_EQ(controls.Movement().second, -1.f);
		EXPECT_TRUE(controls.Up());
		EXPECT_EQ(controls.ConsumeLook(), (std::pair<float, float>{20.f, 30.f}));
		controls.Release(3);
		EXPECT_FALSE(controls.Up());
		EXPECT_FLOAT_EQ(controls.Movement().second, -1.f);
	}

	TEST(TouchControlsTest, MovementIsBoundedAndReleasedIndependently)
	{
		TouchControls controls;
		controls.Resize(1000.f, 600.f);
		controls.Down(1, 100.f, 400.f);
		controls.Move(1, 10000.f, -10000.f);
		const auto [x, y] = controls.Movement();
		EXPECT_NEAR(std::hypot(x, y), 1.f, 0.0001f);
		controls.Release(99);
		EXPECT_EQ(controls.Movement(), (std::pair<float, float>{x, y}));
		controls.Release(1);
		EXPECT_EQ(controls.Movement(), (std::pair<float, float>{}));
	}

	TEST(TouchControlsTest, LookAccumulatesWithoutJumpingOnNewPointer)
	{
		TouchControls controls;
		controls.Resize(1000.f, 600.f);
		controls.Down(4, 600.f, 100.f);
		controls.Move(4, 610.f, 110.f);
		controls.Move(4, 620.f, 120.f);
		EXPECT_EQ(controls.ConsumeLook(), (std::pair<float, float>{20.f, 20.f}));
		EXPECT_EQ(controls.ConsumeLook(), (std::pair<float, float>{}));
		controls.Release(4);
		controls.Down(9, 900.f, 200.f);
		EXPECT_EQ(controls.ConsumeLook(), (std::pair<float, float>{}));
		controls.Move(9, 895.f, 203.f);
		EXPECT_EQ(controls.ConsumeLook(), (std::pair<float, float>{-5.f, 3.f}));
	}

	TEST(TouchControlsTest, ResizeAndCancelClearEveryControl)
	{
		TouchControls controls;
		controls.Resize(1000.f, 600.f);
		controls.Down(1, 100.f, 400.f);
		const auto [ux, uy] = controls.UpCenter();
		controls.Down(2, ux, uy);
		controls.Down(3, 600.f, 100.f);
		controls.Move(3, 620.f, 120.f);
		controls.Reset();
		EXPECT_FALSE(controls.Up());
		EXPECT_EQ(controls.Movement(), (std::pair<float, float>{}));
		EXPECT_EQ(controls.ConsumeLook(), (std::pair<float, float>{}));
		controls.Down(2, ux, uy);
		controls.Resize(600.f, 1000.f);
		EXPECT_FALSE(controls.Up());
	}

	TEST(TouchControlsTest, ExtraPointerDoesNotStealLook)
	{
		TouchControls controls;
		controls.Resize(1000.f, 600.f);
		controls.Down(1, 600.f, 100.f);
		controls.Down(2, 800.f, 200.f);
		controls.Move(2, 850.f, 200.f);
		EXPECT_EQ(controls.ConsumeLook(), (std::pair<float, float>{}));
		controls.Move(1, 605.f, 100.f);
		EXPECT_FLOAT_EQ(controls.ConsumeLook().first, 5.f);
	}
}
