import {
    Button,
    theme,
} from "antd";
import {EyeOutlined} from "@ant-design/icons";
import usePrestoSearchState from "../../SearchState/Presto";

const OpenQueryDrawerButton = () => {
    const {token} = theme.useToken();
    const updateQueryDrawerOpen = usePrestoSearchState((s) => s.updateQueryDrawerOpen);

    return (
        <Button
            icon={
                <EyeOutlined
                    style={{
                        color: token.colorTextSecondary,
                    }}
                />
            }
            size={"small"}
            variant={"filled"}
            color={"default"}
            onClick={() => updateQueryDrawerOpen(true)}
        />
    );
};

export default OpenQueryDrawerButton;
