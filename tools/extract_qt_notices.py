"""Extract Qt notices, not its unrelated source/test tree, for Windows deployment."""

import argparse
import json
import posixpath
import tarfile
from pathlib import Path, PurePosixPath


def extract_notices(archive_path: Path, destination: Path) -> int:
    destination = destination.resolve()
    with tarfile.open(archive_path, "r:xz") as archive:
        members = {}
        for member in archive.getmembers():
            parts = PurePosixPath(member.name).parts
            if len(parts) > 1:
                members[PurePosixPath(*parts[1:]).as_posix()] = member

        def read(name):
            if name not in members:
                raise ValueError(f"Required notice not in archive: {name}")
            stream = archive.extractfile(members[name])
            if stream is None:
                raise ValueError(f"Required notice is not a file: {name}")
            with stream:
                return stream.read()

        selected = {name for name, member in members.items()
                    if name.startswith("LICENSES/") and member.isfile()}
        attributions = [name for name in members
                        if PurePosixPath(name).name == "qt_attribution.json"]
        for name in sorted(attributions, key=lambda item: members[item].offset):
            selected.add(name)
            # Qt attribution files can contain literal multiline descriptions,
            # also accepted by the packager's ConvertFrom-Json reader.
            entries = json.loads(read(name), strict=False)
            if isinstance(entries, dict):
                entries = [entries]
            for entry in entries:
                files = entry.get("LicenseFile") or []
                if isinstance(files, str):
                    files = [files]
                for filename in files:
                    filename = filename.replace("\\", "/")
                    relative = posixpath.normpath(posixpath.join(posixpath.dirname(name), filename))
                    if relative.startswith("../") or PurePosixPath(relative).is_absolute():
                        raise ValueError(f"Notice escapes source module: {filename}")
                    selected.add(relative)

        for name in sorted(selected, key=lambda item: members[item].offset if item in members else -1):
            target = destination.joinpath(*PurePosixPath(name).parts).resolve()
            if not target.is_relative_to(destination):
                raise ValueError(f"Notice escapes destination: {name}")
            data = read(name)
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        if not selected:
            raise ValueError("Archive contains no Qt notices")
        return len(selected)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    print(f"Prepared {extract_notices(args.archive, args.destination)} Qt notice files", flush=True)
