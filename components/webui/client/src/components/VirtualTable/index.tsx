import React, {
    useCallback,
    useRef,
} from "react";

import {Empty, Table} from "antd";

import {
    SCROLL_INCREMENT,
    VIRTUAL_TABLE_HOLDER_SELECTOR,
    type VirtualTableProps,
} from "./typings";

interface FullHeightEmptyProps {
    height: number;
}

/**
 * Empty component that fills the specified height.
 */
const FullHeightEmpty = ({height}: FullHeightEmptyProps) => (
    <div
        style={{
            display: "flex",
            alignItems: "center",
            justifyContent: "center",
            height: height,
        }}
    >
        <Empty image={Empty.PRESENTED_IMAGE_SIMPLE}/>
    </div>
);


/**
 * Virtual table that supports keyboard navigation.
 *
 * @param props
 * @param props.tableProps
 * @return
 */
const VirtualTable = <RecordType extends object = Record<string, unknown>>({
    scroll,
    locale,
    ...tableProps
}: VirtualTableProps<RecordType>) => {
    const containerRef = useRef<HTMLDivElement>(null);

    const handleKeyDown = useCallback((e: React.KeyboardEvent<HTMLDivElement>) => {
        if (null === containerRef.current) {
            return;
        }

        const scrollNode = containerRef.current.querySelector<HTMLElement>(
            VIRTUAL_TABLE_HOLDER_SELECTOR
        );

        if (null === scrollNode) {
            return;
        }

        const visibleTableHeight = scrollNode.clientHeight;
        let {scrollTop} = scrollNode;

        switch (e.key) {
            case "ArrowDown":
                scrollTop += SCROLL_INCREMENT;
                break;
            case "ArrowUp":
                // Prevent scrolling past the top.
                scrollTop = Math.max(scrollTop - SCROLL_INCREMENT, 0);
                break;
            case "PageDown":
                scrollTop += visibleTableHeight;
                break;
            case "PageUp":
                // Prevent scrolling past the top.
                scrollTop = Math.max(scrollTop - visibleTableHeight, 0);
                break;
            case "Home":
                scrollTop = 0;
                break;
            case "End":
                // Scroll to the bottom of the table.
                scrollTop = Number.MAX_SAFE_INTEGER;
                break;
            default:
                return;
        }

        scrollNode.scrollTop = scrollTop;
        e.preventDefault();
    }, []);

    // Use scroll.y height for the empty state if available
    const emptyHeight = "number" === typeof scroll?.y ? scroll.y : undefined;

    // Build locale with full-height empty state
    const fullHeightLocale = emptyHeight ?
        {
            ...locale,
            emptyText: <FullHeightEmpty height={emptyHeight}/>,
        } :
        locale;

    return (
        <div
            ref={containerRef}
            style={{outline: "none"}}
            tabIndex={0}
            onKeyDown={handleKeyDown}
        >
            <Table<RecordType>
                virtual={true}
                {...tableProps}
                {...(scroll && {scroll})}
                {...(fullHeightLocale && {locale: fullHeightLocale})}/>
        </div>
    );
};

export default VirtualTable;
