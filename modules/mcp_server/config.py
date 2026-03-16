def can_build(env, platform):
    env.module_add_dependencies("mcp_server", ["jsonrpc", "websocket"], True)
    return True


def configure(env):
    pass


def get_doc_classes():
    return ["MCPServer"]


def get_doc_path():
    return "doc_classes"
