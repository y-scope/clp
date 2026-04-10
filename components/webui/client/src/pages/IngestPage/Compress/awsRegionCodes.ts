import partitions from "@aws-sdk/util-endpoints/dist-es/lib/aws/partitions.json" with {
    type: "json"};


/**
 * S3-compatible region codes extracted from the AWS SDK endpoints data.
 *
 * Global pseudo-regions (e.g. `aws-global`, `aws-cn-global`) are excluded.
 */
const AWS_REGION_CODES: string[] = Object.keys(
    partitions.partitions.find((p) => "aws" === p.id)?.regions ?? {},
);

export {AWS_REGION_CODES};
