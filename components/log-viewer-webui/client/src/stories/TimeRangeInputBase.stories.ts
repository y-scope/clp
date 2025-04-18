import type {
    Meta,
    StoryObj,
} from "@storybook/react";
import {
    expect,
    fn,
    screen,
    userEvent,
    within,
} from "@storybook/test";
import dayjs from "dayjs";

import TimeRangeInputBase, {TIME_RANGE_OPTION} from "../components/TimeRangeInputBase";
import {TIME_RANGE_OPTION_NAMES} from "../components/TimeRangeInputBase/utils";


const meta = {
    component: TimeRangeInputBase,
    title: "Search/TimeRangeInputBase",

    parameters: {
        layout: "centered",
    },
    tags: ["autodocs"],

    argTypes: {
        defaultValue: {
            options: TIME_RANGE_OPTION_NAMES,
            mapping: TIME_RANGE_OPTION,
            control: {
                type: "select",
            },
        },
    },

    args: {
        defaultValue: TIME_RANGE_OPTION.TODAY,
        onChange: fn(),
    },
} satisfies Meta<typeof TimeRangeInputBase>;

type Story = StoryObj<typeof meta>;


const Custom: Story = {
    args: {
        defaultValue: TIME_RANGE_OPTION.CUSTOM,
        onChange: fn(),
    },
    play: async ({canvasElement, args}) => {
        const canvas = within(canvasElement);
        const startDateInput = await canvas.findByPlaceholderText("Start date");
        const endDateInput = await canvas.findByPlaceholderText("End date");

        await userEvent.clear(startDateInput);
        await userEvent.keyboard("1970-01-01 00:00:00{enter}");
        await userEvent.clear(endDateInput);
        await userEvent.keyboard(dayjs().format("YYYY-MM-DD HH:mm:ss"));
        await expect(args.onChange).toHaveBeenCalledTimes(0);
        await userEvent.keyboard("{enter}");
        await expect(args.onChange).toHaveBeenCalledTimes(1);
    },
};

const Default: Story = {
    args: {
        defaultValue: TIME_RANGE_OPTION.TODAY,
        onChange: fn(),
    },
};

const SelectPreset: Story = {
    args: {
        defaultValue: TIME_RANGE_OPTION.LAST_15_MINUTES,
        onChange: fn(),
    },
    play: async ({canvasElement, args}) => {
        const canvas = within(canvasElement);
        const select = await canvas.findByRole("combobox");

        await userEvent.click(select);
        for (const option of TIME_RANGE_OPTION_NAMES) {
            const matches = await screen.findAllByText(option);
            await expect(matches.length).toBeGreaterThan(0);
        }

        await expect(args.onChange).toHaveBeenCalledTimes(0);
        await userEvent.click(screen.getByText(TIME_RANGE_OPTION.LAST_7_DAYS));
        await expect(args.onChange).toHaveBeenCalledTimes(1);
    },
};

const SelectCustom: Story = {
    args: {
        defaultValue: TIME_RANGE_OPTION.LAST_15_MINUTES,
        onChange: fn(),
    },
    play: async ({canvasElement, args}) => {
        const canvas = within(canvasElement);
        const select = await canvas.findByRole("combobox");

        await userEvent.click(select);
        await userEvent.click(screen.getByText(TIME_RANGE_OPTION.CUSTOM));

        await expect(args.onChange).toHaveBeenCalledTimes(0);
    },
};


export {
    Custom,
    Default,
    SelectCustom,
    SelectPreset,
};
export default meta;
