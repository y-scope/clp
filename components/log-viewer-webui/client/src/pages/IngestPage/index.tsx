import {
    Col,
    Row,
} from "antd";

import styles from "./index.module.css";
import SpaceSavings from "./SpaceSavings";


/**
 * Presents compression statistics.
 *
 * @return
 */
const IngestPage = () => {
    return (
        <div className={styles["ingestPageGrid"]}>
            <Row>
                <Col span={8}>
                    <SpaceSavings/>
                </Col>
                <Col span={8}/>
                <Col span={8}/>
            </Row>
        </div>
    );
};


export default IngestPage;
