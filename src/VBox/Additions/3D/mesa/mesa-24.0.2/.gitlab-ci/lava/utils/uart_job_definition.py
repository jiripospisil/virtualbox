from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    from ..lava_job_submitter import LAVAJobSubmitter
    from .lava_job_definition import LAVAJobDefinition

from .constants import NUMBER_OF_ATTEMPTS_LAVA_BOOT

# Use the same image that is being used for the hardware enablement and health-checks.
# They are pretty small (<100MB) and have all the tools we need to run LAVA, so it is a safe choice.
# You can find the Dockerfile here:
# https://gitlab.collabora.com/lava/health-check-docker/-/blob/main/Dockerfile
# And the registry here: https://gitlab.collabora.com/lava/health-check-docker/container_registry/
DOCKER_IMAGE = "registry.gitlab.collabora.com/lava/health-check-docker"


def fastboot_deploy_actions(
    job_definition: "LAVAJobDefinition", nfsrootfs
) -> tuple[dict[str, Any], ...]:
    args = job_definition.job_submitter
    fastboot_deploy_nfs = {
        "timeout": {"minutes": 10},
        "to": "nfs",
        "nfsrootfs": nfsrootfs,
    }

    fastboot_deploy_prepare = {
        "timeout": {"minutes": 5},
        "to": "downloads",
        "os": "oe",
        "images": {
            "kernel": {
                "url": f"{args.kernel_url_prefix}/{args.kernel_image_name}",
            },
        },
        "postprocess": {
            "docker": {
                "image": DOCKER_IMAGE,
                "steps": [
                    f"cat Image.gz {args.dtb_filename}.dtb > Image.gz+dtb",
                    "mkbootimg --kernel Image.gz+dtb"
                    + ' --cmdline "root=/dev/nfs rw nfsroot=$NFS_SERVER_IP:$NFS_ROOTFS,tcp,hard rootwait ip=dhcp init=/init"'
                    + " --pagesize 4096 --base 0x80000000 -o boot.img",
                ],
            }
        },
    }

    fastboot_deploy = {
        "timeout": {"minutes": 2},
        "to": "fastboot",
        "docker": {
            "image": DOCKER_IMAGE,
        },
        "images": {
            "boot": {"url": "downloads://boot.img"},
        },
    }

    # URLs to our kernel rootfs to boot from, both generated by the base
    # container build
    job_definition.attach_kernel_and_dtb(fastboot_deploy_prepare["images"])
    job_definition.attach_external_modules(fastboot_deploy_nfs)

    return (fastboot_deploy_nfs, fastboot_deploy_prepare, fastboot_deploy)


def tftp_deploy_actions(job_definition: "LAVAJobDefinition", nfsrootfs) -> tuple[dict[str, Any]]:
    args = job_definition.job_submitter
    tftp_deploy = {
        "timeout": {"minutes": 5},
        "to": "tftp",
        "os": "oe",
        "kernel": {
            "url": f"{args.kernel_url_prefix}/{args.kernel_image_name}",
        },
        "nfsrootfs": nfsrootfs,
    }
    job_definition.attach_kernel_and_dtb(tftp_deploy)
    job_definition.attach_external_modules(tftp_deploy)

    return (tftp_deploy,)


def uart_test_actions(
    args: "LAVAJobSubmitter", init_stage1_steps: list[str], artifact_download_steps: list[str]
) -> tuple[dict[str, Any]]:
    # skeleton test definition: only declaring each job as a single 'test'
    # since LAVA's test parsing is not useful to us
    run_steps = []
    test = {
        "timeout": {"minutes": args.job_timeout_min},
        "failure_retry": 1,
        "definitions": [
            {
                "name": "mesa",
                "from": "inline",
                "lava-signal": "kmsg",
                "path": "inline/mesa.yaml",
                "repository": {
                    "metadata": {
                        "name": "mesa",
                        "description": "Mesa test plan",
                        "os": ["oe"],
                        "scope": ["functional"],
                        "format": "Lava-Test Test Definition 1.0",
                    },
                    "run": {"steps": run_steps},
                },
            }
        ],
    }

    run_steps += init_stage1_steps
    run_steps += artifact_download_steps

    run_steps += [
        f"mkdir -p {args.ci_project_dir}",
        f"curl {args.build_url} | tar --zstd -x -C {args.ci_project_dir}",
        # Sleep a bit to give time for bash to dump shell xtrace messages into
        # console which may cause interleaving with LAVA_SIGNAL_STARTTC in some
        # devices like a618.
        "sleep 1",
        # Putting CI_JOB name as the testcase name, it may help LAVA farm
        # maintainers with monitoring
        f"lava-test-case '{args.project_name}_{args.mesa_job_name}' --shell /init-stage2.sh",
    ]

    return (test,)


def tftp_boot_action(args: "LAVAJobSubmitter") -> dict[str, Any]:
    tftp_boot = {
        "failure_retry": NUMBER_OF_ATTEMPTS_LAVA_BOOT,
        "method": args.boot_method,
        "prompts": ["lava-shell:"],
        "commands": "nfs",
    }

    return tftp_boot


def fastboot_boot_action(args: "LAVAJobSubmitter") -> dict[str, Any]:
    fastboot_boot = {
        "timeout": {"minutes": 2},
        "docker": {"image": DOCKER_IMAGE},
        "failure_retry": NUMBER_OF_ATTEMPTS_LAVA_BOOT,
        "method": args.boot_method,
        "prompts": ["lava-shell:"],
        "commands": ["set_active a"],
    }

    return fastboot_boot
