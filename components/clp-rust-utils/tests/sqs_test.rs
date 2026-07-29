use clp_rust_utils::sqs::event::S3;

#[rustfmt::skip]
/// Example S3 messages copied from AWS documentation:
/// <https://docs.aws.amazon.com/AmazonS3/latest/userguide/notification-content-structure.html#notification-content-structure-examples>
const _EXAMPLE_DOC: () = ();

const EXAMPLE_S3_PUT_MESSAGE: &str = r#"{
   "Records":[
      {
         "eventVersion":"2.1",
         "eventSource":"aws:s3",
         "awsRegion":"us-west-2",
         "eventTime":"1970-01-01T00:00:00.000Z",
         "eventName":"ObjectCreated:Put",
         "userIdentity":{
            "principalId":"AIDAJDPLRKLG7UEXAMPLE"
         },
         "requestParameters":{
            "sourceIPAddress":"172.16.0.1"
         },
         "responseElements":{
            "x-amz-request-id":"C3D13FE58DE4C810",
            "x-amz-id-2":"FMyUVURIY8/IgAtTv8xRjskZQpcIZ9KG4V5Wp6S7S/JRWeUWerMUE5JgHvANOjpD"
         },
         "s3":{
            "s3SchemaVersion":"1.0",
            "configurationId":"testConfigRule",
            "bucket":{
               "name":"amzn-s3-demo-bucket",
               "ownerIdentity":{
                  "principalId":"A3NL1KOZZKExample"
               },
               "arn":"arn:aws:s3:::amzn-s3-demo-bucket"
            },
            "object":{
               "key":"HappyFace.jpg",
               "size":1024,
               "eTag":"d41d8cd98f00b204e9800998ecf8427e",
               "versionId":"096fKKXTRTtl3on89fVO.nfljtsv6qko",
               "sequencer":"0055AED6DCD90281E5"
            }
         }
      }
   ]
}"#;

const EXAMPLE_S3_TEST_MESSAGE: &str = r#"{
   "Service":"Amazon S3",
   "Event":"s3:TestEvent",
   "Time":"2014-10-13T15:57:02.089Z",
   "Bucket":"amzn-s3-demo-bucket",
   "RequestId":"5582815E1AEA5ADF",
   "HostId":"8cLeGAmw098X5cv4Zkwcmo8vvZa3eH3eKxsPzbB9wrR+YstdA6Knx4Ip8EXAMPLE"
}"#;

#[test]
fn test_s3_event_deserialization() {
    let s3_put_event: S3 =
        serde_json::from_str(EXAMPLE_S3_PUT_MESSAGE).expect("Deserialized S3 PUT event");
    assert_eq!(s3_put_event.records.len(), 1);
    let record = &s3_put_event.records[0];
    assert_eq!(record.event_name, "ObjectCreated:Put");
    assert_eq!(record.s3.bucket.name, "amzn-s3-demo-bucket");
    assert_eq!(record.s3.object.key, "HappyFace.jpg");
    assert_eq!(record.s3.object.size, 1024);

    serde_json::from_str::<S3>(EXAMPLE_S3_TEST_MESSAGE)
        .expect_err("S3 Test event should fail deserialization");
}
