# Configuring IRSA for Amazon EKS

When deploying CLP on [Amazon EKS][eks] with `type: "default"` authentication, you must configure
[IAM Roles for Service Accounts (IRSA)][irsa] to grant CLP pods access to AWS S3.

:::{note}
EKS Auto Mode blocks EC2 instance metadata (IMDS) from pods, so the AWS SDK's default credential
provider chain relies on IRSA web identity tokens instead.
:::

---

## Prerequisites

* An EKS cluster with CLP deployed via Helm (see the
  [Kubernetes deployment guide](../../guides-k8s-deployment.md))
* CLP configured with `type: "default"` authentication (see 
  [Configuring CLP](clp-config.md#default))
* An IAM policy granting S3 access (see [Configuring AWS S3](aws-s3-config.md))

---

## Step 1: Get the OIDC provider URL

```bash
aws eks describe-cluster --name <cluster-name> --region <region> \
  --query "cluster.identity.oidc.issuer" --output text
```

This outputs a URL like `https://oidc.eks.<region>.amazonaws.com/id/<OIDC_ID>`.

## Step 2: Register the OIDC provider in IAM

1. In the AWS Console, go to **IAM** > **Identity providers** > **Add provider**
2. **Provider type**: OpenID Connect
3. **Provider URL**: paste the URL from Step 1
4. **Audience**: `sts.amazonaws.com`
5. Click **Add provider**

## Step 3: Create an IAM role for IRSA

Create a role with a trust policy that allows CLP's ServiceAccount to assume it. Replace
`<ACCOUNT_ID>`, `<OIDC_ID>`, and `<namespace>` with the appropriate values:

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Principal": {
        "Federated": "arn:aws:iam::<ACCOUNT_ID>:oidc-provider/oidc.eks.<region>.amazonaws.com/id/<OIDC_ID>"
      },
      "Action": "sts:AssumeRoleWithWebIdentity",
      "Condition": {
        "StringEquals": {
          "oidc.eks.<region>.amazonaws.com/id/<OIDC_ID>:aud": "sts.amazonaws.com"
        },
        "StringLike": {
          "oidc.eks.<region>.amazonaws.com/id/<OIDC_ID>:sub": "system:serviceaccount:<namespace>:*"
        }
      }
    }
  ]
}
```

:::{tip}
Using `system:serviceaccount:<namespace>:*` allows all ServiceAccounts in the namespace to assume
the role. For tighter access control, specify the exact ServiceAccount name:
`system:serviceaccount:<namespace>:<release>-clp-service-account`.
:::

Attach the S3 IAM policy (from [Configuring AWS S3](aws-s3-config.md)) to this role.

## Step 4: Configure the role ARN in Helm values

Set the IRSA role ARN in your Helm values file so the ServiceAccount annotation is applied at
install time:

```yaml
serviceAccount:
  annotations:
    eks.amazonaws.com/role-arn: "arn:aws:iam::<ACCOUNT_ID>:role/<role-name>"
```

Then install (or upgrade) the Helm chart:

```bash
helm install <release> clp/clp -f values.yaml
```

## Verifying IRSA

To confirm that a pod has IRSA credentials:

```bash
kubectl exec <pod-name> -- env | grep AWS_ROLE_ARN
```

The output should show the ARN of the role you created.

[eks]: https://aws.amazon.com/eks/
[irsa]: https://docs.aws.amazon.com/eks/latest/userguide/iam-roles-for-service-accounts.html
