import {
    Col,
    Row,
    Card,
} from "antd";

import styles from "./index.module.css";
import SpaceSavings from "./SpaceSavings";
import Details from "./Details"; // Added missing import

/**
 * Presents compression statistics.
 *
 * @return
 */
const IngestPage = () => {
    return (
        <div className={styles["ingestPageGrid"]}>
            <Row gutter={16} className={styles["row"]}>
                <Col span={8} className={styles["col"]}>
                    <SpaceSavings />
                </Col>
                <Col span={8} className={styles["col"]}>
                    <Details />
                </Col>
            </Row>
        </div>
    );
};

export default IngestPage;
