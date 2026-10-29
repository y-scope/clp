import React from "react";

import {
    MinusOutlined,
    PlusOutlined,
} from "@ant-design/icons";
import {TreeSelectProps} from "antd";


type SwitcherIconFn = Exclude<TreeSelectProps["switcherIcon"], React.ReactNode>;

type SwitcherIconProps = Parameters<NonNullable<SwitcherIconFn>>[0];


/**
 * Icon component for tree node expand/collapse state.
 *
 * @param props
 * @param props.expanded Whether the node is expanded.
 * @return
 */
const SwitcherIcon = ({expanded}: SwitcherIconProps) => (expanded ?
    <MinusOutlined style={{color: "grey"}}/> :
    <PlusOutlined style={{color: "grey"}}/>);


export default SwitcherIcon;
