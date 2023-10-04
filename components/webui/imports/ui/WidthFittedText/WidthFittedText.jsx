import React, {useRef} from "react";

import useWindowResizeMonitor from "../useWindowResizeMonitor/useWindowResizeMonitor";

const WidthFittedText = ({text, widthDeps}) => {
    const windowDimensions = useWindowResizeMonitor();
    const componentContainerRef = useRef(null);
    const textContainerRef = useRef(null);

    let [fontSize, setFontSize] = React.useState(14);

    const deps = [text, windowDimensions].concat(widthDeps);
    React.useEffect(() => {
        const textContainerRect = textContainerRef.current.getBoundingClientRect();
        const componentContainerRect = componentContainerRef.current.getBoundingClientRect();
        if (textContainerRect.width < componentContainerRect.width - 2 || textContainerRect.width > componentContainerRect.width + 2) {
            setFontSize(textContainerRect.height / textContainerRect.width * componentContainerRect.width);
        }
    }, deps);

    return (
        <div ref={componentContainerRef}>
            <div className="wft-wrapper">
                <div className="wft-text-container" ref={textContainerRef} style={{fontSize: fontSize + "px", lineHeight: fontSize + "px"}}>{text}</div>
            </div>
        </div>
    );
}

export default WidthFittedText;
