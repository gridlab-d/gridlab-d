function glm_write(glm,obj,indent)
if ~exist('indent')
    indent = '';
end
if ~isfield(obj,'class')
    error 'class is a required field';
end
fprintf(glm,'%sobject %s {\n',indent,obj.class);
for f = fieldnames(obj)';
    prop = char(f);
    if strcmp(prop,'class')
        % ignore this
    elseif strcmp(prop,'child')
        for m = 1:length(obj.child)
            glm_write(glm,obj.child{m},[indent,'    ']);
        end
    else
        fprintf(glm,'%s    %s %s;\n', indent,prop, obj.(prop));
    end
end
fprintf(glm,'%s};\n',indent);
